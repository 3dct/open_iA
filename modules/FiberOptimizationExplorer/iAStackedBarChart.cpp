/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAStackedBarChart.h"

#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iAMathUtility.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

namespace
{
	double MinimumWeight = 0.001;
	int MinimumPixelBarWidth = 1;
}

iAStackedBarChart::iAStackedBarChart(iAColorTheme const * theme, bool header):
	m_theme(theme),
	m_contextMenu(new QMenu(this)),
	m_header(header),
	m_stack(true),
	m_resizeBar(-1),
	m_resizeStartX(0)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	QAction* switchStack = new QAction("Switch Stacked Mode", nullptr);
	switchStack->setCheckable(true);
	switchStack->setChecked(true);
	connect(switchStack, &QAction::triggered, this, &iAStackedBarChart::switchStackMode);
	m_contextMenu->addAction(switchStack);
}

void iAStackedBarChart::addBar(QString const & name, double value, double maxValue)
{
	m_bars.push_back(iABarData(name, value, maxValue, (m_bars.size()==0) ? 1 : 1.0/m_bars.size()));
	normalizeWeights();
	update();
}

void iAStackedBarChart::removeBar(QString const & name)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](iABarData const & d){ return d.name == name; });
	if (it != m_bars.end())
		m_bars.erase(it);
	normalizeWeights();
	update();
}

void iAStackedBarChart::setColorTheme(iAColorTheme const * theme)
{
	m_theme = theme;
}

QMenu* iAStackedBarChart::contextMenu()
{
	return m_contextMenu;
}

void iAStackedBarChart::setDoStack(bool doStack)
{
	m_stack = doStack;
}

void iAStackedBarChart::switchStackMode()
{
	// TODO: Log interaction
	QAction* sender = qobject_cast<QAction*>(QObject::sender());
	setDoStack(sender->isChecked());
	update();
	emit switchedStackMode(sender->isChecked());
}

void iAStackedBarChart::paintEvent(QPaintEvent* ev)
{
	m_dividers.clear();
	QPainter painter(this);
	painter.setPen(QColor(0, 0, 0));
	int accumulatedWidth = 0;
	int barHeight = std::min(geometry().height(), MaxBarHeight);
	int topY = geometry().height() / 2 - barHeight / 2;
	painter.fillRect(geometry(), QBrush(QWidget::palette().color(QWidget::backgroundRole())));
	for (size_t barID = 0; barID < m_bars.size(); ++barID)
	{
		auto & bar = m_bars[barID];
		int bWidth = barWidth(bar);
		QRect barRect(accumulatedWidth, topY, bWidth, barHeight);
		QBrush barBrush(m_theme->GetColor(barID));
		painter.fillRect(barRect, barBrush);
		barRect.adjust(TextPadding, 0, -TextPadding, 0);
		painter.drawText(barRect, Qt::AlignVCenter,
			(m_header ? bar.name : QString::number(bar.value)));
		m_dividers.push_back(accumulatedWidth + bWidth);
		accumulatedWidth += m_stack ? bWidth : geometry().width() / m_bars.size();
	}
}

void iAStackedBarChart::contextMenuEvent(QContextMenuEvent *ev)
{
	if (m_header)
		m_contextMenu->exec(ev->globalPos());
}

int iAStackedBarChart::dividerWithinRange(int x) const
{
	for (size_t divID = 0; divID < m_dividers.size(); ++divID)
		if (abs(m_dividers[divID] - x) < DividerRange)
			return divID;
	return -1;
}

void iAStackedBarChart::normalizeWeights()
{
	double weightSum = 0;
	for (auto & bar : m_bars)
		weightSum += bar.weight;
	for (auto & bar : m_bars)
		bar.weight = std::max(MinimumWeight, bar.weight/weightSum);
}

int iAStackedBarChart::barWidth(iABarData const & bar) const
{
	return std::max(MinimumPixelBarWidth, static_cast<int>((bar.value / bar.maxValue) * bar.weight * geometry().width()) );
}

void iAStackedBarChart::mouseMoveEvent(QMouseEvent* ev)
{
	if (m_header)
	{
		if (m_resizeBar != -1)
		{
			if (!(ev->buttons() & Qt::LeftButton))  // left button was released without being in the window?
			{
				DEBUG_LOG("iAStackedBarChart: resizedBar set but left button not pressed! Resetting...");
				m_resizeBar = -1;
			}
			else
			{
				int xOfs = ev->x() - m_resizeStartX;
				double newWidth = clamp(1.0, static_cast<double>(geometry().width() - m_bars.size() + 1), m_resizeWidth + xOfs);
				double oldRestWidth = geometry().width() - m_resizeWidth;
				double newRestWidth = geometry().width() - newWidth;
				//DEBUG_LOG(QString("width: %1; resize bar: %2; old width: %3, newWidth: %4, old rest width: %5, new rest width: %6")
				//	.arg(geometry().width()).arg(m_resizeBar).arg(m_resizeWidth).arg(newWidth).arg(oldRestWidth).arg(newRestWidth));
				for (size_t barID = 0; barID < m_bars.size(); ++barID)
				{
					m_bars[barID].weight = std::max(MinimumWeight, m_resizeBars[barID].weight *
						((barID == m_resizeBar)? (newWidth / m_resizeWidth) : (newRestWidth / oldRestWidth)) );
					//DEBUG_LOG(QString("    Bar %1: %2").arg(barID).arg(m_bars[barID].weight));
				}
				update();
			}
		}
		else
		{
			int barID = dividerWithinRange(ev->x());
			this->setCursor(barID >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
		}
	}
	else
	{
		int curBar = 0;
		while (curBar < m_dividers.size() && ev->x() > m_dividers[curBar])
			++curBar;
		if (curBar < m_bars.size())
		{
			auto & b = m_bars[curBar];
			QToolTip::showText(ev->globalPos(), QString("%1: %2 (weight: %3)").arg(b.name).arg(b.value).arg(b.weight), this);
		}
	}
}

void iAStackedBarChart::mousePressEvent(QMouseEvent* ev)
{
	if (m_header && (ev->button() & Qt::LeftButton))
	{
		int barID = dividerWithinRange(ev->x());
		if (barID != -1)
		{
			m_resizeBar = barID;
			m_resizeStartX = ev->x();
			m_resizeWidth = barWidth(m_bars[barID]);
			m_resizeBars = m_bars;
		}
	}
}

void iAStackedBarChart::mouseReleaseEvent(QMouseEvent* ev)
{
	if (m_resizeBar != -1)
	{
		m_resizeBar = -1;
		std::vector<double> weights(m_bars.size());
		for (size_t b = 0; b < m_bars.size(); ++b)
			weights[b] = m_bars[b].weight;
		emit weightsChanged(weights);
	}
}

void iAStackedBarChart::setWeights(std::vector<double> const & weights)
{
	for (size_t b = 0; b < m_bars.size(); ++b)
		m_bars[b].weight = weights[b];
	update();
}

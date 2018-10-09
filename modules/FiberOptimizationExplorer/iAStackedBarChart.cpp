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

#include <QAction>
#include <QMenu>
#include <QPainter>
#include <QToolTip>

namespace
{
	double MinimumWeight = 0.01;
	int MinimumPixelBarWidth = 1;
}

class iABarData
{
public:
	iABarData(QString const & name, double value, double maxValue): 
		name(name), value(value), maxValue(maxValue), weight(1.0)
	{}
	QString name;
	double value, maxValue, weight;
};

iAStackedBarChart::iAStackedBarChart(iAColorTheme const * theme, bool header) :
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
	m_bars.push_back(iABarData(name, value, maxValue));
	update();
}

void iAStackedBarChart::removeBar(QString const & name)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](iABarData const & d){ return d.name == name; });
	if (it != m_bars.end())
		m_bars.erase(it);
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
	double widthPerWeightFactor = widthPerWeight();
	for (size_t barID = 0; barID < m_bars.size(); ++barID)
	{
		auto & bar = m_bars[barID];
		int bWidth = barWidth(bar, widthPerWeightFactor);
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

double iAStackedBarChart::widthPerWeight() const
{
	double weightSum = 0;
	for (auto & bar : m_bars)
		weightSum += bar.weight;
	return static_cast<double>(geometry().width()) / weightSum;
}

int iAStackedBarChart::barWidth(iABarData const & bar, double widthPerWeight) const
{
	return std::max(MinimumPixelBarWidth, static_cast<int>((bar.value / bar.maxValue) * bar.weight * widthPerWeight));
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
				int xOfs = m_resizeStartX - ev->x();
				double bw = barWidth(m_bars[m_resizeBar], widthPerWeight());
				double  newBW = bw + xOfs;
				m_resizeStartX = ev->x();
				m_bars[m_resizeBar].weight = std::max(MinimumWeight, m_bars[m_resizeBar].weight * (bw/newBW));
				DEBUG_LOG(QString("Bar %1: old width: %2, new width: %3, new weight: %4").arg(m_resizeBar).arg(bw).arg(newBW).arg(m_bars[m_resizeBar].weight));
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
			//QToolTip::showText(ev->globalPos(), QString("%1: %2 (weight: %3)").arg(b.name).arg(b.value).arg(b.weight), this);
			setToolTip(QString("%1: %2 (weight: %3)").arg(b.name).arg(b.value).arg(b.weight));
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
		}
	}
}

void iAStackedBarChart::mouseReleaseEvent(QMouseEvent* ev)
{
	if (m_resizeBar != -1)
	{
		emit weightChanged(m_resizeBar, m_bars[m_resizeBar].weight);
		m_resizeBar = -1;
	}
}

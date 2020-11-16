/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iAColorTheme.h>
#include <iALog.h>
#include <iAMathUtility.h>

#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include <limits>

namespace
{
	double MinimumWeight = 0.001;
	int MinimumPixelBarWidth = 1;
	const int DividerRange = 2;
	const int BarSpacing = 1;
	size_t NoBar = std::numeric_limits<size_t>::max();
}

iAStackedBarChart::iAStackedBarChart(iAColorTheme const * theme, bool header, bool last):
	m_theme(theme),
	m_contextMenu(header ? new QMenu(this): nullptr),
	m_header(header),
	m_last(last),
	m_stack(true),
	m_resizeBar(NoBar),
	m_resizeStartX(0),
	m_resizeWidth(0),
	m_normalizePerBar(true),
	m_selectedBar(-1)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	// Context Menu:
	QAction* switchStack = new QAction("Stacked mode", nullptr);
	switchStack->setCheckable(true);
	switchStack->setChecked(true);
	connect(switchStack, &QAction::triggered, this, &iAStackedBarChart::switchStackMode);

	QAction* resetWeightsAction = new QAction("Set equal weights", nullptr);
	connect(resetWeightsAction, &QAction::triggered, this, &iAStackedBarChart::resetWeights);

	QAction* normalizeAction = new QAction("Normalize per bar", nullptr);
	normalizeAction->setCheckable(true);
	normalizeAction->setChecked(true);
	connect(normalizeAction, &QAction::triggered, this, &iAStackedBarChart::toggleNormalizeMode);
	if (m_header)
	{
		m_contextMenu->addAction(switchStack);
		m_contextMenu->addAction(normalizeAction);
		m_contextMenu->addAction(resetWeightsAction);
		m_contextMenu->addSeparator();
	}
}

void iAStackedBarChart::addBar(QString const & name, double value, double maxValue)
{
	m_bars.push_back(iABarData(name, value, maxValue, (m_bars.size()==0) ? 1 : 1.0/m_bars.size()));
	normalizeWeights();
	update();
}

void iAStackedBarChart::updateBar(QString const& name, double value, double maxValue)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](iABarData const& d) { return d.name == name; });
	if (it != m_bars.end())
	{
		it->value = value;
		it->maxValue = maxValue;
	}
	updateOverallMax();
}

void iAStackedBarChart::removeBar(QString const & name)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](iABarData const & d){ return d.name == name; });
	if (it != m_bars.end())
	{
		m_bars.erase(it);
	}
	normalizeWeights();
	update();
}

int iAStackedBarChart::barIndex(QString const& name) const
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](iABarData const& d) { return d.name == name; });
	return it == m_bars.end() ? -1 : it - m_bars.begin();
}

void iAStackedBarChart::setColorTheme(iAColorTheme const * theme)
{
	m_theme = theme;
	update();
}

QMenu* iAStackedBarChart::contextMenu()
{
	return m_contextMenu;
}

void iAStackedBarChart::setDoStack(bool doStack)
{
	if (m_contextMenu)
	{
		m_contextMenu->actions()[0]->setChecked(doStack);
	}
	m_stack = doStack;
	update();
}

void iAStackedBarChart::setNormalizeMode(bool normalizePerBar)
{
	m_normalizePerBar = normalizePerBar;
	if (m_contextMenu)
	{
		m_contextMenu->actions()[1]->setChecked(normalizePerBar);
	}
	update();
}

size_t iAStackedBarChart::numberOfBars() const
{
	return m_bars.size();
}

double iAStackedBarChart::weightAndNormalize(iABarData const& bar) const
{
	return bar.value /
		(m_normalizePerBar ? bar.maxValue : m_overallMaxValue) * bar.weight;
}

double iAStackedBarChart::weightedSum() const
{
	double result = 0;
	for (auto const & b: m_bars)
	{
		result += weightAndNormalize(b);
	}
	return result;
}

void iAStackedBarChart::setSelectedBar(int barIdx)
{
	m_selectedBar = barIdx;
	update();
}

QString iAStackedBarChart::barName(int barIdx) const
{
	return m_bars[barIdx].name;
}

void iAStackedBarChart::setBackgroundColor(QColor const & color)
{
	m_bgColor = color;
	update();
}

void iAStackedBarChart::switchStackMode()
{
	QAction* sender = qobject_cast<QAction*>(QObject::sender());
	m_stack = sender->isChecked(); // don't use setDoStack, as this sets checked state, and would therefore trigger recursive signal
	update();
	emit switchedStackMode(sender->isChecked());
}

void iAStackedBarChart::paintEvent(QPaintEvent* /*ev*/)
{
	m_dividers.clear();
	QPainter painter(this);
	painter.setPen(QWidget::palette().color(QPalette::Text));
	int accumulatedWidth = 0;
	int barHeight = std::min(geometry().height(), MaxBarHeight) - (m_header? 0 : 2*BarSpacing);
	int topY = geometry().height() / 2 - barHeight / 2;
	QColor bg(m_bgColor);
	if (!bg.isValid())
	{
		bg = QWidget::palette().color(QWidget::backgroundRole());
	}
	painter.fillRect(rect(), QBrush(bg));
	for (size_t barID = 0; barID < m_bars.size(); ++barID)
	{
		auto & bar = m_bars[barID];
		int bWidth = barWidth(bar);
		QRect barRect(accumulatedWidth, topY, bWidth, barHeight);
		QBrush barBrush(m_theme->color(barID));
		painter.fillRect(barRect, barBrush);
		if (m_selectedBar == barID)
		{
			QRect box(accumulatedWidth, 0,
				(m_stack ? bWidth : static_cast<int>(bar.weight * geometry().width())) - 1, geometry().height());
			if (m_header)
			{
				painter.drawLine(box.topLeft(), box.topRight());
			}
			painter.drawLine(box.topLeft(), box.bottomLeft());
			painter.drawLine(box.topRight(), box.bottomRight());
			if (m_last)
			{
				painter.drawLine(box.bottomLeft(), box.bottomRight());
			}
		}
		barRect.adjust(TextPadding, 0, -TextPadding, 0);
		painter.drawText(barRect, Qt::AlignVCenter,
			(m_header ? bar.name : QString("%1").arg(bar.value)));
		m_dividers.push_back(accumulatedWidth + bWidth);
		accumulatedWidth += m_stack ? bWidth : static_cast<int>(bar.weight * geometry().width());
	}
}

void iAStackedBarChart::contextMenuEvent(QContextMenuEvent *ev)
{
	if (m_header)
	{
		m_contextMenu->exec(ev->globalPos());
	}
}

size_t iAStackedBarChart::dividerWithinRange(int x) const
{
	for (size_t divID = 0; divID < m_dividers.size(); ++divID)
	{
		if (abs(m_dividers[divID] - x) < DividerRange)
		{
			return divID;
		}
	}
	return NoBar;
}

void iAStackedBarChart::normalizeWeights()
{
	updateOverallMax();
	double weightSum = 0;
	for (auto& bar : m_bars)
	{
		weightSum += bar.weight;
	}
	for (auto& bar : m_bars)
	{
		bar.weight = std::max(MinimumWeight, bar.weight / weightSum);
	}
}

void iAStackedBarChart::updateOverallMax()
{
	m_overallMaxValue = std::numeric_limits<double>::lowest();
	for (auto& bar : m_bars)
	{
		if (bar.maxValue > m_overallMaxValue)
		{
			m_overallMaxValue = bar.maxValue;
		}
	}
}

int iAStackedBarChart::barWidth(iABarData const & bar) const
{
	return std::max(MinimumPixelBarWidth,
		static_cast<int>(weightAndNormalize(bar) * geometry().width()) );
}

void iAStackedBarChart::mouseMoveEvent(QMouseEvent* ev)
{
	if (m_header)
	{
		if (m_resizeBar != NoBar)
		{
			if (!(ev->buttons() & Qt::LeftButton))  // left button was released without being in the window?
			{
				LOG(lvlError, "iAStackedBarChart: resizedBar set but left button not pressed! Resetting...");
				m_resizeBar = NoBar;
			}
			else
			{
				int xOfs = ev->x() - m_resizeStartX;
				double newWidth = clamp(1.0, static_cast<double>(geometry().width() - m_bars.size() + 1), m_resizeWidth + xOfs);
				double oldRestWidth = geometry().width() - m_resizeWidth;
				double newRestWidth = geometry().width() - newWidth;
				//LOG(lvlInfo, QString("width: %1; resize bar: %2; old width: %3, newWidth: %4, old rest width: %5, new rest width: %6")
				//	.arg(geometry().width()).arg(m_resizeBar).arg(m_resizeWidth).arg(newWidth).arg(oldRestWidth).arg(newRestWidth));
				for (size_t barID = 0; barID < m_bars.size(); ++barID)
				{
					m_bars[barID].weight = std::max(MinimumWeight, m_resizeBars[barID].weight *
						((barID == m_resizeBar)? (newWidth / m_resizeWidth) : (newRestWidth / oldRestWidth)) );
					//LOG(lvlInfo, QString("    Bar %1: %2").arg(barID).arg(m_bars[barID].weight));
				}
				update();
			}
		}
		else
		{
			size_t barID = dividerWithinRange(ev->x());
			this->setCursor(barID != NoBar ? Qt::SizeHorCursor : Qt::ArrowCursor);
		}
	}
	else
	{
		size_t curBar = 0;
		while (curBar < m_dividers.size() && ev->x() > m_dividers[curBar])
		{
			++curBar;
		}
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
		size_t barID = dividerWithinRange(ev->x());
		if (barID != NoBar)
		{
			m_resizeBar = barID;
			m_resizeStartX = ev->x();
			m_resizeWidth = barWidth(m_bars[barID]);
			m_resizeBars = m_bars;
		}
	}
}

void iAStackedBarChart::mouseReleaseEvent(QMouseEvent* /*ev*/)
{
	if (m_resizeBar != NoBar)
	{
		m_resizeBar = NoBar;
		std::vector<double> weights(m_bars.size());
		for (size_t b = 0; b < m_bars.size(); ++b)
		{
			weights[b] = m_bars[b].weight;
		}
		emit weightsChanged(weights);
	}
	else
	{
		emit clicked();
	}
}

void iAStackedBarChart::mouseDoubleClickEvent(QMouseEvent* ev)
{
	emit doubleClicked();
	int x = ev->x();
	int barID = -1;
	for (size_t divID = 0; divID < m_dividers.size(); ++divID)
	{
		if (x >= ((divID > 0) ? m_dividers[divID - 1] : 0) && x < m_dividers[divID])
		{
			barID = static_cast<int>(divID);
		}
	}
	if (barID != -1)
	{
		emit barDblClicked(barID);
	}
}

void iAStackedBarChart::setWeights(std::vector<double> const & weights)
{
	for (size_t b = 0; b < m_bars.size(); ++b)
	{
		m_bars[b].weight = weights[b];
	}
	update();
}

void iAStackedBarChart::resetWeights()
{
	std::vector<double> weights(m_bars.size(), 1.0 / m_bars.size());
	for (size_t b = 0; b < m_bars.size(); ++b)
	{
		m_bars[b].weight = weights[b];
	}
	update();
	emit weightsChanged(weights);
}

void iAStackedBarChart::toggleNormalizeMode()
{
	m_normalizePerBar = !m_normalizePerBar; // don't use setNormalizeMode, as this sets checked state, and would therefore trigger recursive signal
	update();
	emit normalizeModeChanged(m_normalizePerBar);
}

void addHeaderLabel(QGridLayout* layout, int column, QString const& text)
{
	auto headerLabel = new QLabel(text);
	headerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	layout->addWidget(headerLabel, 0, column);
}
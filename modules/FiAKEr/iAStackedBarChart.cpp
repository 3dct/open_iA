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
#include <iAStringHelper.h>

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
	const int BarVSpacing = 1;
	const int BarHSpacing = 2;
	size_t NoBar = std::numeric_limits<size_t>::max();
	const int MaxChartWidth = 150;
}

class iABarData
{
public:
	iABarData() : name(""), value(0), maxValue(1), minValDiff(1), weight(1.0)
	{
	}
	iABarData(QString const& name, double value, double maxValue, double minValDiff, double weight) :
		name(name),
		value(value),
		maxValue(maxValue),
		minValDiff(minValDiff),
		weight(weight)
	{
	}
	~iABarData()
	{
	}
	QString name;
	double value, maxValue, minValDiff, weight;
};

class iABarsWidget: public QWidget
{
public:
	iABarsWidget(iAStackedBarChart* s):
		m_s(s)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	QSize sizeHint() const
	{
		return QSize(10, fontMetrics().lineSpacing());  // font height?
	}
	void contextMenuEvent(QContextMenuEvent* ev) override
	{
		m_s->contextMenuEvent(ev);
	}
	void resizeEvent(QResizeEvent* e)
	{
		QWidget::resizeEvent(e);
		m_s->updateDividers();
	}
	void mouseDoubleClickEvent(QMouseEvent* event) override
	{
		size_t barID = m_s->getBarAt(event->x());
		LOG(lvlDebug, QString("DblClicked on bar %1").arg(barID));
		m_s->emitBarClick(barID);
	}
	void mousePressEvent(QMouseEvent* event) override
	{
		size_t barID = m_s->getBarAt(event->x());
		LOG(lvlDebug, QString("Clicked on bar %1").arg(barID));
		m_s->emitBarDblClick(barID);
	}

private:
	void paintEvent(QPaintEvent* ev) override;

private:
	iAStackedBarChart* m_s;
};

class iABarWidget: public QWidget
{
public:
	iABarWidget(iAStackedBarChart* s, int barID) : m_barID(barID), m_s(s)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	}
	QSize sizeHint() const
	{
		return QSize(10, fontMetrics().lineSpacing());  // font height?
	}
	void contextMenuEvent(QContextMenuEvent* ev) override
	{
		m_s->contextMenuEvent(ev);
	}
	void resizeEvent(QResizeEvent* e)
	{
		QWidget::resizeEvent(e);
		m_s->updateDividers();
	}
	void mouseDoubleClickEvent(QMouseEvent* ev) override
	{
		Q_UNUSED(ev);
		m_s->emitBarClick(m_barID);
	}
	void mousePressEvent(QMouseEvent* ev) override
	{
		Q_UNUSED(ev);
		m_s->emitBarDblClick(m_barID);
	}
	void paintEvent(QPaintEvent* ev) override
	{
		Q_UNUSED(ev);
		QPainter painter(this);
		QColor bg(m_s->m_bgColor);
		if (!bg.isValid())
		{
			bg = QWidget::palette().color(QWidget::backgroundRole());
		}
		painter.fillRect(rect(), QBrush(bg));
		m_s->drawBar(painter, m_barID, m_barID == 0 ? m_s->m_leftMargin : 0, 0,
			std::min(geometry().height(), iAStackedBarChart::MaxBarHeight) - (m_s->m_header ? 0 : 2 * BarVSpacing)
		);
	}
	int m_barID;
	iAStackedBarChart* m_s;
};

iAStackedBarChart::iAStackedBarChart(iAColorTheme const* theme, QGridLayout* gL, int row, int col,
	bool header, bool last) :
	m_theme(theme),
	m_contextMenu(header ? new QMenu(this) : nullptr),
	m_header(header),
	m_last(last),
	m_stack(true),
	m_resizeBar(NoBar),
	m_resizeStartX(0),
	m_resizeWidth(0),
	m_normalizePerBar(true),
	m_overallMaxValue(0),
	m_selectedBar(-1),
	m_leftMargin(0),
	m_barsWidget(new iABarsWidget(this)),
	m_gL(gL),
	m_row(row),
	m_col(col)
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

void iAStackedBarChart::addBar(QString const & name, double value, double maxValue, double minValDiff)
{
	m_bars.push_back(QSharedPointer<iABarData>::create(name, value, maxValue, minValDiff, (m_bars.size()==0) ? 1 : 1.0/m_bars.size()));
	// add bars widget / update row span if it already is added:
	if (m_stack)
	{
		m_gL->addWidget(m_barsWidget, m_row, m_col, 1, static_cast<int>(m_bars.size()));
	}
	else
	{
		m_gL->addWidget(new iABarWidget(this, static_cast<int>(m_bars.size() - 1)), m_row,
			m_col + static_cast<int>(m_bars.size() - 1));
	}
	updateColumnStretch();
	normalizeWeights();
	updateDividers();
}

void iAStackedBarChart::emitBarClick(size_t barID)
{
	emit clicked();
	emit barClicked(barID);
}

void iAStackedBarChart::emitBarDblClick(size_t barID)
{
	emit dblClicked();
	emit barDblClicked(barID);
}

void iAStackedBarChart::updateBar(QString const& name, double value, double maxValue, double minValDiff)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](QSharedPointer<iABarData> d) { return d->name == name; });
	if (it != m_bars.end())
	{
		(*it)->value = value;
		(*it)->maxValue = maxValue;
		(*it)->minValDiff = minValDiff;
	}
	updateColumnStretch();
	updateOverallMax();
	updateDividers();
}

int iAStackedBarChart::removeBar(QString const & name)
{
	int barIdx = barIndex(name);
	assert(barIdx != -1);
	m_bars.erase(m_bars.begin() + barIdx);
	if (m_stack)
	{
		// update row span of bars widget:
		m_gL->addWidget(m_barsWidget, m_row, m_col, 1, static_cast<int>(m_bars.size()));
	}
	else
	{	// always delete last -> just the bar IDs need to be ordered, then the correct data is shown
		auto w = m_gL->itemAtPosition(m_row, static_cast<int>(m_col + m_bars.size()))->widget();
		m_gL->removeWidget(w);
		m_gL->setColumnStretch(static_cast<int>(m_col + m_bars.size()), 0);
		delete w;
	}
	normalizeWeights();
	updateColumnStretch();
	updateDividers();
	return barIdx;
}

int iAStackedBarChart::barIndex(QString const& name) const
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](QSharedPointer<iABarData> d) { return d->name == name; });
	return it == m_bars.end() ? -1 : it - m_bars.begin();
}

void iAStackedBarChart::setColorTheme(iAColorTheme const * theme)
{
	m_theme = theme;
	updateBars();
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
	if (m_stack)
	{
		// update row span of bars widget:
		m_gL->addWidget(m_barsWidget, m_row, m_col, 1, static_cast<int>(m_bars.size()));
	}
	else
	{
		m_gL->removeWidget(m_barsWidget);
		for (int i = 0; i < m_bars.size(); ++i)
		{
			m_gL->addWidget(new iABarWidget(this, i), m_row, m_col + i);
		}
	}
	updateBars();
}

void iAStackedBarChart::setNormalizeMode(bool normalizePerBar)
{
	m_normalizePerBar = normalizePerBar;
	if (m_contextMenu)
	{
		m_contextMenu->actions()[1]->setChecked(normalizePerBar);
	}
	updateBars();
}

void iAStackedBarChart::updateBars()
{
	if (m_stack)
	{
		m_barsWidget->update();
	}
	else
	{
		for (int i = 0; i < m_bars.size(); ++i)
		{
			m_gL->itemAtPosition(m_row, m_col + i)->widget()->update();
		}
	}
}

size_t iAStackedBarChart::getBarAt(int x) const
{
	size_t barID = 0;
	while (barID < m_dividers.size() && x > m_dividers[barID])
	{
		++barID;
	}
	return barID;
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
		result += weightAndNormalize(*b.data());
	}
	return result;
}

void iAStackedBarChart::setSelectedBar(int barIdx)
{
	m_selectedBar = barIdx;
	updateBars();
}

QString iAStackedBarChart::barName(size_t barIdx) const
{
	return m_bars[barIdx]->name;
}

void iAStackedBarChart::setLeftMargin(int leftMargin)
{
	m_leftMargin = leftMargin;
}

void iAStackedBarChart::setBackgroundColor(QColor const& color)
{
	m_bgColor = color;
	updateBars();
}

void iAStackedBarChart::switchStackMode()
{
	QAction* sender = qobject_cast<QAction*>(QObject::sender());
	m_stack = sender->isChecked(); // don't use setDoStack, as this sets checked state, and would therefore trigger recursive signal
	updateBars();
	emit switchedStackMode(sender->isChecked());
}


void iAStackedBarChart::resizeEvent(QResizeEvent* e)
{
	QWidget::resizeEvent(e);
	updateDividers();
}

void iAStackedBarChart::drawBar(QPainter& painter, size_t barID, int left, int top, int barHeight)
{
	assert(barID < m_bars.size());
	auto& bar = m_bars[barID];
	int bWidth = barWidth(*bar.data());
	QRect barRect(left, top, bWidth, barHeight);
	QBrush barBrush(m_theme->color(barID));
	painter.fillRect(barRect, barBrush);
	int segmentWidth = (m_stack ? bWidth : static_cast<int>(bar->weight * (m_chartAreaPixelWidth - m_leftMargin))) - 1;
	QRect segmentBox(left, 0, segmentWidth, barHeight);
	if (m_selectedBar == barID)
	{
		if (m_header)
		{
			painter.drawLine(segmentBox.topLeft(), segmentBox.topRight());
		}
		painter.drawLine(segmentBox.topLeft(), segmentBox.bottomLeft());
		painter.drawLine(segmentBox.topRight(), segmentBox.bottomRight());
		if (m_last)
		{
			painter.drawLine(segmentBox.bottomLeft(), segmentBox.bottomRight());
		}
	}
	segmentBox.adjust(iAStackedBarChart::TextPadding, 0, -iAStackedBarChart::TextPadding, 0);
	painter.drawText(segmentBox, Qt::AlignVCenter,
		(m_header ? bar->name : QString::number(bar->value, 'f', digitsAfterComma(bar->minValDiff))));
}

void iABarsWidget::paintEvent(QPaintEvent* ev)
{
	Q_UNUSED(ev);
	QPainter painter(this);
	QColor bg(m_s->m_bgColor);
	if (!bg.isValid())
	{
		bg = QWidget::palette().color(QWidget::backgroundRole());
	}
	painter.fillRect(rect(), QBrush(bg));
	painter.setPen(QWidget::palette().color(QPalette::Text));
	int accumulatedWidth = 0;
	int barHeight =
		std::min(geometry().height(), iAStackedBarChart::MaxBarHeight) - (m_s->m_header ? 0 : 2 * BarVSpacing);
	int topY = geometry().height() / 2 - barHeight / 2;
	for (size_t barID = 0; barID < m_s->m_bars.size(); ++barID)
	{
		auto& bar = m_s->m_bars[barID];
		int bWidth = m_s->barWidth(*bar.data());
		m_s->drawBar(painter, barID, accumulatedWidth + m_s->m_leftMargin, topY,
			std::min(geometry().height(), iAStackedBarChart::MaxBarHeight) - (m_s->m_header ? 0 : 2 * BarVSpacing)
			);
		accumulatedWidth +=
			m_s->m_stack ? bWidth : static_cast<int>(bar->weight * (m_s->m_chartAreaPixelWidth - m_s->m_leftMargin));
	}
}

void iAStackedBarChart::updateDividers()
{
	m_dividers.clear();
	if (m_stack)
	{
		m_chartAreaPixelWidth = m_barsWidget->geometry().width();
		int accumulatedWidth = m_leftMargin;
		for (size_t barID = 0; barID < m_bars.size(); ++barID)
		{
			accumulatedWidth += barWidth(*m_bars[barID].data());
			m_dividers.push_back(accumulatedWidth);
		}
	}
	else
	{
		m_chartAreaPixelWidth = 0;
		for (int i = 0; i < m_bars.size(); ++i)
		{
			m_chartAreaPixelWidth += m_gL->itemAtPosition(m_row, m_col + i)->widget()->geometry().width();
		}
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
		weightSum += bar->weight;
	}
	for (auto& bar : m_bars)
	{
		bar->weight = std::max(MinimumWeight, bar->weight / weightSum);
	}
}

void iAStackedBarChart::updateOverallMax()
{
	m_overallMaxValue = std::numeric_limits<double>::lowest();
	for (auto& bar : m_bars)
	{
		if (bar->maxValue > m_overallMaxValue)
		{
			m_overallMaxValue = bar->maxValue;
		}
	}
}

void iAStackedBarChart::updateColumnStretch()
{
	for (size_t barID = 0; barID < m_bars.size(); ++barID)
	{
		auto& bar = m_bars[barID];
		int chartStretch = std::min(MaxChartWidth, static_cast<int>(bar->weight * m_chartAreaPixelWidth))
			+ ((barID == 0) ? m_leftMargin : 0);
		m_gL->setColumnStretch(static_cast<int>(m_col+barID), chartStretch);
	}
}

int iAStackedBarChart::barWidth(iABarData const & bar) const
{
	return std::max(MinimumPixelBarWidth, static_cast<int>(
		weightAndNormalize(bar) * (m_chartAreaPixelWidth - m_leftMargin) ));
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
					m_bars[barID]->weight = std::max(MinimumWeight,
						m_resizeBars[barID]->weight *
							((barID == m_resizeBar) ? (newWidth / m_resizeWidth)
														 : (newRestWidth / oldRestWidth)));
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
		size_t curBar = getBarAt(ev->x());
		if (curBar < m_bars.size())
		{
			auto& b = m_bars[curBar];
			QToolTip::showText(ev->globalPos(), QString("%1: %2 (weight: %3)")
				.arg(b->name).arg(b->value).arg(b->weight), this);
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
			m_resizeWidth = barWidth(*m_bars[barID].data());
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
			weights[b] = m_bars[b]->weight;
		}
		//emit weightsChanged(weights);
	}
	else
	{
		//emit clicked();
	}
}

void iAStackedBarChart::setWeights(std::vector<double> const & weights)
{
	for (size_t b = 0; b < m_bars.size(); ++b)
	{
		m_bars[b]->weight = weights[b];
	}
	updateBars();
}

void iAStackedBarChart::resetWeights()
{
	std::vector<double> weights(m_bars.size(), 1.0 / m_bars.size());
	for (size_t b = 0; b < m_bars.size(); ++b)
	{
		m_bars[b]->weight = weights[b];
	}
	updateBars();
	emit weightsChanged(weights);
}

void iAStackedBarChart::toggleNormalizeMode()
{
	m_normalizePerBar = !m_normalizePerBar; // don't use setNormalizeMode, as this sets checked state, and would therefore trigger recursive signal
	updateBars();
	emit normalizeModeChanged(m_normalizePerBar);
}

void addHeaderLabel(QGridLayout* layout, int column, QString const& text)
{
	auto headerLabel = new QLabel(text);
	headerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	layout->addWidget(headerLabel, 0, column);
}
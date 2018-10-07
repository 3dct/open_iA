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

iAStackedBarChart::iAStackedBarChart(iAColorTheme const * theme, bool header):
	m_theme(theme),
	m_contextMenu(new QMenu(this)),
	m_header(header),
	m_stack(true)
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
	m_bars.push_back(std::make_tuple(name, value, maxValue));
}

void iAStackedBarChart::removeBar(QString const & name)
{
	auto it = std::find_if(m_bars.begin(), m_bars.end(),
		[name](BarData const & d){ return std::get<0>(d) == name; });
	if (it != m_bars.end())
		m_bars.erase(it);
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
	DEBUG_LOG(QString("%1 x %2").arg(geometry().width()).arg(geometry().height()));
	QPainter painter(this);
	painter.setPen(QColor(0, 0, 0));
	int accumulatedWidth = 0;
	int barHeight = std::min(geometry().height(), MaxBarHeight);
	int topY = geometry().height() / 2 - barHeight / 2;
	painter.fillRect(geometry(), QBrush(QWidget::palette().color(QWidget::backgroundRole())));
	for (size_t barID = 0; barID < m_bars.size(); ++barID)
	{
		int barWidth = (std::get<1>(m_bars[barID])/std::get<2>(m_bars[barID]))*geometry().width() / m_bars.size();
		DEBUG_LOG(QString("    value=%1, maxValue=%2, barWidth=%3")
				  .arg(std::get<1>(m_bars[barID]))
				  .arg(std::get<2>(m_bars[barID]))
				  .arg(barWidth));
		QRect barRect(accumulatedWidth, topY, barWidth, barHeight);
		QBrush barBrush(m_theme->GetColor(barID));
		painter.fillRect(barRect, barBrush);
		barRect.adjust(TextPadding, 0, -TextPadding, 0);
		painter.drawText(barRect, Qt::AlignVCenter,
			(m_header ? std::get<0>(m_bars[barID]) : QString::number(std::get<1>(m_bars[barID]))));
		m_dividers.push_back(accumulatedWidth+barWidth);
		accumulatedWidth += m_stack ? barWidth : geometry().width() / m_bars.size();
	}
}

void iAStackedBarChart::contextMenuEvent(QContextMenuEvent *ev)
{
	if (m_header)
		m_contextMenu->exec(ev->globalPos());
}

void iAStackedBarChart::mouseMoveEvent(QMouseEvent* ev)
{
	if (m_header)
	{
		const int DividerRange = 5;
		int dividerWithinRange = -1;
		for (size_t divID = 0; divID < m_dividers.size(); ++divID)
			if ( abs(m_dividers[divID]- ev->x()) < DividerRange )
				dividerWithinRange = divID;
		this->setCursor( dividerWithinRange >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor );
	}
	else
	{
		int curBar = 0;
		while (curBar < m_dividers.size() && ev->x() > m_dividers[curBar])
			++curBar;
		if (curBar < m_dividers.size())
		{
			auto & b = m_bars[curBar];
			QToolTip::showText(ev->globalPos(), QString("%1: %2").arg(std::get<0>(b)).arg(std::get<1>(b)), this);
		}
	}
}

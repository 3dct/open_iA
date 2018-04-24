/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAMemberView.h"

#include "iAUncertaintyColors.h"
#include "iAEnsemble.h"
#include "iAMember.h"

#include "charts/qcustomplot.h"

#include <QHBoxLayout>

#include <vector>

template <typename T>
std::vector<size_t> sort_indices_desc(const std::vector<T> &v) {

	// initialize original index locations
	std::vector<size_t> idx(v.size());
	std::iota(idx.begin(), idx.end(), 0);

	// sort indexes based on comparing values in v
	std::sort(idx.begin(), idx.end(),
		[&v](size_t i1, size_t i2) {return v[i1] > v[i2]; });

	return idx;
}


iAMemberView::iAMemberView():
	m_plot(new QCustomPlot())
{
	m_plot->setOpenGl(true);
	setLayout(new QHBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(4, 4, 4, 4);
	layout()->addWidget(m_plot);
	m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_plot->setMultiSelectModifier(Qt::ShiftModifier);
	connect(m_plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(ChartMousePress(QMouseEvent *)));
}

void iAMemberView::SetEnsemble(QSharedPointer<iAEnsemble> ensemble)
{
	m_plot->clearPlottables();
	m_ensemble = ensemble;

	m_sortedIndices.clear();
	m_sortedIndices = sort_indices_desc<double>(ensemble->MemberAttribute(iAEnsemble::UncertaintyMean));

	mean = new QCPBars(m_plot->xAxis, m_plot->yAxis);
	mean->setPen(QPen(iAUncertaintyColors::MemberBar));
	mean->setBrush(QBrush(iAUncertaintyColors::MemberBar));
	mean->setName("Mean Uncertainty");
	mean->setSelectable(QCP::stMultipleDataRanges);
	mean->selectionDecorator()->setPen(iAUncertaintyColors::SelectedMember);
	mean->selectionDecorator()->setBrush(iAUncertaintyColors::SelectedMember);

	QVector<double> ticks;
	QVector<QString> labels;
	QVector<double> data;

	size_t cnt = 0;
	for (double idx : m_sortedIndices)
	{
		ticks << cnt;
		labels << QString::number(static_cast<int>(ensemble->Member(idx)->ID()));
		data << ensemble->MemberAttribute(iAEnsemble::UncertaintyMean)[idx];
		++cnt;
	}

	QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
	textTicker->addTicks(ticks, labels);
	m_plot->xAxis->setTicker(textTicker);
	m_plot->xAxis->setLabel("Member ID");
	m_plot->xAxis->setRange(-1, 20); // by default, show the first 20 member...
	m_plot->yAxis->setLabel("Mean Uncertainty");
	m_plot->yAxis->setRange(0, 1);
	m_plot->axisRect()->setRangeDrag(Qt::Horizontal); // ... but allow dragging
	m_plot->axisRect()->setRangeZoom(Qt::Horizontal); // and zooming in horizontal direction
	mean->setData(ticks, data);

	connect(mean, SIGNAL(selectionChanged(QCPDataSelection const &)), this, SLOT(SelectionChanged(QCPDataSelection const &)));
	connect(m_plot->xAxis, SIGNAL(rangeChanged(const QCPRange &)), this, SLOT(ChangedRange(QCPRange const &)));

	StyleChanged();
	m_plot->replot();
}

void iAMemberView::ChangedRange(QCPRange const & newRange)
{
	double lowerBound = -1;
	double upperBound = m_ensemble->MemberAttribute(iAEnsemble::UncertaintyMean).size();
	QCPRange fixedRange(newRange);
	if (fixedRange.lower < lowerBound)
	{
		fixedRange.lower = lowerBound;
		fixedRange.upper = lowerBound + newRange.size();
		if (fixedRange.upper > upperBound || qFuzzyCompare(newRange.size(), upperBound - lowerBound))
			fixedRange.upper = upperBound;
		m_plot->xAxis->setRange(fixedRange);
	}
	else if (fixedRange.upper > upperBound)
	{
		fixedRange.upper = upperBound;
		fixedRange.lower = upperBound - newRange.size();
		if (fixedRange.lower < lowerBound || qFuzzyCompare(newRange.size(), upperBound - lowerBound))
			fixedRange.lower = lowerBound;
		m_plot->xAxis->setRange(fixedRange);
	}
}


void iAMemberView::ChartMousePress(QMouseEvent *)
{
	if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{	// allow selection with Ctrl key
		m_plot->setSelectionRectMode(QCP::srmSelect);
	}
	else
	{	// enable dragging otherwise
		m_plot->setSelectionRectMode(QCP::srmNone);
	}
}

void iAMemberView::SelectionChanged(QCPDataSelection const & selection)
{
	if (selection.dataRangeCount() == 1 && selection.dataRange(0).begin()+1 == selection.dataRange(0).end())
	{
		int barIdx = selection.dataRange(0).begin();
		emit MemberSelected(m_ensemble->Member(m_sortedIndices[barIdx])->ID());
	}
}

QVector<int > iAMemberView::SelectedMemberIDs() const
{
	QCPDataSelection selection = mean->selection();
	QVector<int> result;
	for (int r = 0; r < selection.dataRangeCount(); ++r)
	{
		for (int barIdx = selection.dataRange(r).begin(); barIdx < selection.dataRange(r).end(); ++barIdx)
		{
			result.push_back(m_ensemble->Member(m_sortedIndices[barIdx])->ID());
		}
	}
	return result;
}


void iAMemberView::StyleChanged()
{
	QColor bg(QWidget::palette().color(QPalette::Background));
	QColor fg(QWidget::palette().color(QPalette::Text));
	m_plot->setBackground(bg);
	m_plot->axisRect()->setBackground(bg);
	for (auto a : m_plot->axisRect()->axes())
	{
		a->setLabelColor(fg);
		a->setTickLabelColor(fg);
		a->setBasePen(fg);
		a->setTickPen(fg);
	}
	m_plot->replot();
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMemberView.h"

#include "iAUncertaintyColors.h"
#include "iAEnsemble.h"
#include "iASingleResult.h"

#include <qcustomplot.h>

#include <QApplication>
#include <QGuiApplication>
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
	connect(m_plot, &QCustomPlot::mousePress, this, &iAMemberView::ChartMousePress);
	connect(m_plot, &QCustomPlot::mouseWheel, this, &iAMemberView::mouseWheel);
}

void iAMemberView::SetEnsemble(std::shared_ptr<iAEnsemble> ensemble)
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
	QVector<double> meanData;

	size_t cnt = 0;
	for (double idx : m_sortedIndices)
	{
		ticks << cnt;
		labels << QString::number(static_cast<int>(ensemble->Member(idx)->id()));
		meanData << ensemble->MemberAttribute(iAEnsemble::UncertaintyMean)[idx];
		++cnt;
	}

	auto textTicker = QSharedPointer<QCPAxisTickerText>::create();
	textTicker->addTicks(ticks, labels);
	m_plot->xAxis->setTicker(textTicker);
	m_plot->xAxis->setLabel("Member ID");
	m_plot->xAxis->setRange(-1, 20); // by default, show the first 20 member...
	m_plot->yAxis->setLabel("Mean Uncertainty");
	m_plot->yAxis->setRange(0, 1);
	m_plot->axisRect()->setRangeDrag(Qt::Horizontal); // ... but allow dragging
	m_plot->axisRect()->setRangeZoom(Qt::Horizontal); // and zooming in horizontal direction
	mean->setData(ticks, meanData);

	connect(mean, QOverload<QCPDataSelection const&>::of(&QCPBars::selectionChanged), this, &iAMemberView::SelectionChanged);
	connect(m_plot->xAxis, QOverload<const QCPRange&>::of(&QCPAxis::rangeChanged), this, &iAMemberView::ChangedRange);

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
		emit MemberSelected(m_ensemble->Member(m_sortedIndices[barIdx])->id());
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
			result.push_back(m_ensemble->Member(m_sortedIndices[barIdx])->id());
		}
	}
	return result;
}

void iAMemberView::StyleChanged()
{
	QColor bg(QApplication::palette().color(QPalette::Window));
	QColor fg(QApplication::palette().color(QPalette::Text));
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

void iAMemberView::mouseWheel(QWheelEvent* e)
{
	switch (e->modifiers())
	{
	case Qt::AltModifier:
		m_plot->axisRect()->setRangeZoom(Qt::Vertical);
		break;
	default:
		m_plot->axisRect()->setRangeZoom(Qt::Horizontal);
		break;
	}
}

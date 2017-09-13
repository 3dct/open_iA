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
#include "iAMemberView.h"

#include "iAColors.h"
#include "iAEnsemble.h"

#include <QHBoxLayout>

#include "qcustomplot.h"

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
	setLayout(new QHBoxLayout());
	layout()->addWidget(m_plot);
}

void iAMemberView::SetEnsemble(QSharedPointer<iAEnsemble> ensemble)
{
	m_ensemble = ensemble;

	auto sortedIndices = sort_indices_desc<double>(ensemble->MemberAttribute(iAEnsemble::UncertaintyMean));

	QCPBars * mean = new QCPBars(m_plot->xAxis, m_plot->yAxis);
	mean->setPen(QPen(Uncertainty::MemberBarColor));
	mean->setName("Mean Uncertainty");

	QVector<double> ticks;
	QVector<QString> labels;
	QVector<double> data;

	size_t cnt = 0;
	for (double idx : sortedIndices)
	{
		ticks << cnt;
		labels << QString::number(static_cast<int>(idx));
		data << ensemble->MemberAttribute(iAEnsemble::UncertaintyMean)[idx];
		++cnt;
	}

	QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
	textTicker->addTicks(ticks, labels);
	m_plot->xAxis->setTicker(textTicker);
	m_plot->xAxis->setRange(-1, 20); // by default, show the first 20 member...
	m_plot->yAxis->setLabel("Mean Uncertainty");
	m_plot->yAxis->setRange(0, 1);
	m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
	m_plot->axisRect()->setRangeDrag(Qt::Horizontal); // ... but allow dragging
	m_plot->axisRect()->setRangeZoom(Qt::Horizontal); // and zooming in horizontal direction
	mean->setData(ticks, data);
	connect(m_plot->xAxis, SIGNAL(rangeChanged(const QCPRange &)), this, SLOT(ChangedRange(QCPRange const &)));
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
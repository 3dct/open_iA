/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAHistogramStackGrid.h"

#include "charts/iAChartWithFunctionsWidget.h"
#include "iASimpleSlicerWidget.h"

#include <QGridLayout>
#include <QResizeEvent>
#include <QLabel>

iAHistogramStackGrid::iAHistogramStackGrid(
	QWidget *parent,
	QVector<iAChartWithFunctionsWidget*> const & histograms,
	QVector<iASimpleSlicerWidget*> const & slicers,
	QVector<QLabel*> const & labels,
	Qt::WindowFlags f)

	: QWidget(parent, f)
{
	m_gridLayout = new QGridLayout(this);
	for (int i = 0; i < histograms.size(); i++) {
		m_gridLayout->addWidget(histograms[i], i, 0);
		m_gridLayout->addWidget(slicers[i], i, 1);
		m_gridLayout->addWidget(labels[i], i, 2);
	}
	m_gridLayout->setSpacing(m_spacing);
	m_gridLayout->setMargin(0);
}

void iAHistogramStackGrid::resizeEvent(QResizeEvent* event)
{
	adjustStretch(event->size().width());
}

void iAHistogramStackGrid::adjustStretch(int totalWidth)
{
	//QLayoutItem *item00 = m_gridLayout->itemAtPosition(0, 0);
	//int histogramHeight = item00->widget()->size().height();
	int histogramHeight = size().height() / 3 - 4 * m_spacing;
	m_gridLayout->setColumnStretch(0, totalWidth - histogramHeight);
	m_gridLayout->setColumnStretch(1, histogramHeight);
}
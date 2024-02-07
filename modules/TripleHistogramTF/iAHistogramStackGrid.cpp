// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramStackGrid.h"

#include "iASimpleSlicerWidget.h"

#include <iAChartWithFunctionsWidget.h>

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
	for (int i = 0; i < histograms.size(); i++)
	{
		m_gridLayout->addWidget(histograms[i], i, 0);
		m_gridLayout->addWidget(slicers[i], i, 1);
		m_gridLayout->addWidget(labels[i], i, 2);
	}
	m_gridLayout->setSpacing(m_spacing);
	m_gridLayout->setContentsMargins(0, 0, 0, 0);
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

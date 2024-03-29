// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramView.h"

#include "iAEnsemble.h"

#include <iAChartWidget.h>
#include <iAPlotTypes.h>
#include <iAHistogramData.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QHBoxLayout>

iAHistogramView::iAHistogramView()
{
	setLayout(new QHBoxLayout());
}

void iAHistogramView::AddChart(QString const & caption, QSharedPointer<iAHistogramData> histoData,
		QColor const & color, QSharedPointer<iALookupTable> lut)
{
	m_chart = new iAChartWidget(this, caption, "Frequency (Pixels)");
	m_chart->setMinimumHeight(120);
	auto barGraph = QSharedPointer<iABarGraphPlot>::create(histoData, color, 2);
	barGraph->setLookupTable(lut);
	m_chart->addPlot(barGraph);
	layout()->setSpacing(0);
	layout()->setContentsMargins(4, 4, 4, 4);
	layout()->addWidget(m_chart);
}


void iAHistogramView::Clear()
{
	for (auto widget : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
}

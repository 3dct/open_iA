// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramView.h"

#include "iAEnsemble.h"

#include <iAChartWidget.h>
#include <iAPlotTypes.h>
#include <iAHistogramData.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>

iAHistogramView::iAHistogramView()
{
	setLayout(new QHBoxLayout());
}

void iAHistogramView::AddChart(QString const & caption, std::shared_ptr<iAHistogramData> histoData,
		QColor const & color, std::shared_ptr<iALookupTable> lut)
{
	m_chart = new iAChartWidget(this, caption, "Frequency (Pixels)");
	m_chart->setMinimumHeight(120);
	auto barGraph = std::make_shared<iABarGraphPlot>(histoData, color, 2);
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

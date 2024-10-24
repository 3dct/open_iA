// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramView.h"

#include "iAEnsemble.h"

#include <iAChartWidget.h>
#include <iAHistogramData.h>
#include <iAQWidgetHelper.h>
#include <iAPlotTypes.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>

iAHistogramView::iAHistogramView():
	m_chart(nullptr)
{
	setLayout(createLayout<QHBoxLayout>(0, 4));
}

void iAHistogramView::AddChart(QString const & caption, std::shared_ptr<iAHistogramData> histoData,
		QColor const & color, std::shared_ptr<iALookupTable> lut)
{
	m_chart = new iAChartWidget(this, caption, "Frequency (Pixels)");
	m_chart->setMinimumHeight(120);
	auto barGraph = std::make_shared<iABarGraphPlot>(histoData, color, 2);
	barGraph->setLookupTable(lut);
	m_chart->addPlot(barGraph);
	layout()->addWidget(m_chart);
}


void iAHistogramView::Clear()
{
	for (auto widget : findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
}

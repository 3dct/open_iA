// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkMinimumMaximumImageCalculator.h>
#include <itkImageRegionConstIterator.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <QWidget>

class iAChartWidget;
class iAEnsemble;
class iAHistogramData;

class iALookupTable;

class iAHistogramView : public QWidget
{
	Q_OBJECT
public:
	iAHistogramView();
	void Clear();
	void AddChart(QString const& caption, std::shared_ptr<iAHistogramData> data,
			QColor const & color, std::shared_ptr<iALookupTable> lut = std::shared_ptr<iALookupTable>());
private:
	iAChartWidget* m_chart;
};

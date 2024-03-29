// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkMinimumMaximumImageCalculator.h>
#include <itkImageRegionConstIterator.h>

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
	void AddChart(QString const& caption, QSharedPointer<iAHistogramData> data,
			QColor const & color, QSharedPointer<iALookupTable> lut = QSharedPointer<iALookupTable>());
private:
	iAChartWidget* m_chart;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QList>

//! Computes and stores data required for showing a boxplot.
struct iAguibase_API iABoxPlotData
{
public:
	iABoxPlotData() : q25(0), med(0), q75(0), min(0), max(0) {};
	iABoxPlotData(double q25_v, double med_v, double q75_v, double min_v, double max_v);
	static int cmp(const void *px, const void *py);

	//! Computes values required for the box plot.
	//! @param data the data as array
	//! @param dataSize number of elements in data
	//! @param removeOutliers whether outliers should be removed
	//! @param k used to find outliers as: x < q25 - k * (q75 - q25) || x < q75 - k * ( q75 - q25 )
	void CalculateBoxPlot( double * data, int dataSize, bool removeOutliers = false, double k = 2.0 );

	double q25, med, q75, min, max;
	QList<double> outliers;
	double range[2];
};

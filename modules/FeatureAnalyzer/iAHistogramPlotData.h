// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <QList>

struct iAHistogramPlotData
{
public:
	iAHistogramPlotData();
	static int cmp( const void *px, const void *py );
	void CalculateHistogramPlot( double * data, int dataSize );

	QMultiMap<double, QList<double> > histoBinMap;
	double min, max;
	double range[2];
};

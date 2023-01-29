// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramPlotData.h"

#include "FeatureAnalyzerHelpers.h"

#include <cstdlib>
#include <cstring>

iAHistogramPlotData::iAHistogramPlotData()
{
}

int iAHistogramPlotData::cmp( const void *px, const void *py )
{
	const double *x = (double*) px, *y = (double*) py;
	return ( *x > *y ) - ( *x < *y );
}

void iAHistogramPlotData::CalculateHistogramPlot( double * data, int dataSize )
{
	double * buf_data = new double[dataSize];
	memcpy( buf_data, data, sizeof( double )*dataSize );
	qsort( (void *) buf_data, (size_t) dataSize, sizeof( double ), &iAHistogramPlotData::cmp );

	min = buf_data[0]; max = buf_data[dataSize - 1];
	range[0] = min;
	range[1] = max;

	QList<double> valueList;
	for ( int i = 0; i < dataSize; ++i )
		valueList.append( buf_data[i] );

	delete[] buf_data;

	histoBinMap = calculateHistogram( valueList, 0.0, 100.0 );
}

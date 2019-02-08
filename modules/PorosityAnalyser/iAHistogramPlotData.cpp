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
#include "iAHistogramPlotData.h"

#include "PorosityAnalyserHelpers.h"

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

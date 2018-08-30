/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iABoxPlotData.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

iABoxPlotData::iABoxPlotData( double q25_v, double med_v, double q75_v, double min_v, double max_v )
{
	q25 = q25_v;
	med = med_v;
	q75 = q75_v;
	min = min_v;
	max = max_v;
	range[0] = min;
	range[1] = max;
}

int iABoxPlotData::cmp( const void *px, const void *py )
{
	const double *x = (double*)px, *y = (double*)py;
	return (*x > *y) - (*x < *y);
}

void iABoxPlotData::CalculateBoxPlot( double * data, int dataSize, bool removeOutliers /*= false*/, double k /*= 1.5 */ )
{
	assert(dataSize > 0);
	if (dataSize == 0)
		return;
	double * buf_data = new double[dataSize];
	memcpy( buf_data, data, sizeof(double)*dataSize );
	qsort( (void *)buf_data, dataSize, sizeof( double ), &iABoxPlotData::cmp );
	min = buf_data[0]; max = buf_data[dataSize-1];
	double ind = 0.5*dataSize;
	if( ind != (double)( (int) ind) )
		med = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		med = buf_data[(int)ind];

	ind = 0.25*dataSize;
	if( ind != (double)( (int) ind) )
		q25 = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		q25 = buf_data[(int)ind];

	ind = 0.75*dataSize;
	if( ind != (double)( (int) ind) )
		q75 = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		q75 = buf_data[(int)ind];

	range[0] = min;
	range[1] = max;
	//outliers
	if( !removeOutliers )
		return;
	outliers.clear();
	double outlierRange[2] = { q25 - k * (q75 - q25), q75 + k * (q75 - q25) };
	int outlierInds[2] = { -1, dataSize };
	for( int i = 0; i < 0.25*dataSize; ++i )
		if( buf_data[i] < outlierRange[0] )
		{
			outlierInds[0] = i;
			outliers.push_back( buf_data[i] );
		}
		else
			break;
	for( int i = dataSize - 1; i > 0.75*dataSize; --i )
		if( buf_data[i] > outlierRange[1] )
		{
			outlierInds[1] = i;
			outliers.push_back( buf_data[i] );
		}
		else
			break;

	//recalculate statistics without outliers
	int newIndRange[2] = { outlierInds[0] + 1, outlierInds[1] - 1 };
	int newDataSize = newIndRange[1] - newIndRange[0];
	min = buf_data[ newIndRange[0] ]; max = buf_data[ newIndRange[1] ];
	ind = newIndRange[0] + 0.5*newDataSize;
	if( ind != (double)((int)ind) )
		med = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		med = buf_data[(int)ind];

	ind = newIndRange[0] + 0.25*newDataSize;
	if( ind != (double)((int)ind) )
		q25 = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		q25 = buf_data[(int)ind];

	ind = newIndRange[0] + 0.75*newDataSize;
	if( ind != (double)((int)ind) )
		q75 = 0.5*(buf_data[(int)ind] + buf_data[(int)ind + 1]);
	else
		q75 = buf_data[(int)ind];

	delete [] buf_data;
}

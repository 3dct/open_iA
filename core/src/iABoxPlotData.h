/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#pragma once

#include "open_iA_Core_export.h"

#include <QList>

//! Computes and stores data required for showing a boxplot
struct open_iA_Core_API iABoxPlotData
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

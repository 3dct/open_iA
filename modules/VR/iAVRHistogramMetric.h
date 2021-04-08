/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAVRMetrics.h"

struct HistogramParameters
{
	//featureList needed to map from array position back to feature id
	std::vector<int>* featureList;
	//Stores for every [feature] the min value of both regions
	std::vector<double> minValue;
	//Stores for every [feature] the max value of both regions
	std::vector<double> maxValue;
	//Stores for every [feature] the calculated bin width
	std::vector<double> histogramWidth;
	//Amount of bins for current histogram
	int bins = 0;
	//Stores for every [feature] the occurency in every [bin]
	std::vector<std::vector<int>> histogramRegion1;
	std::vector<std::vector<int>> histogramRegion2;
};

class iAVRHistogramMetric: public iAVRMetrics
{
public:
	iAVRHistogramMetric(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	int getMaxNumberOfHistogramBins(int observations);
	HistogramParameters* getHistogram(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2);

private:
	//Stores the minValue, maxValue, binWidth and bin amount
	HistogramParameters* m_histogramParameter;
	//Stores the currently calculated histogram for the two [regions] and their [bins] with the cumulative number of observations
	std::vector<std::vector<int>>* m_currentHistogram;

	void calculateBinWidth(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2, std::vector<std::vector<std::vector<double>>>* regionValues);
	void calculateHistogramValues(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2);
};


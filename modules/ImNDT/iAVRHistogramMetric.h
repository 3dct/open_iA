/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVRHistogram.h"

//struct HistogramParametersOLD
//{
//	//featureList needed to map from array position back to feature id
//	std::vector<int>* featureList;
//	//Stores for every [feature] the min value of both regions
//	std::vector<double> minValue;
//	//Stores for every [feature] the max value of both regions
//	std::vector<double> maxValue;
//	//Stores for every [feature] the calculated bin width
//	std::vector<double> histogramWidth;
//	//Amount of bins for current histogram
//	int bins = 0;
//	
//	//Stores for every [feature] the occurency in every [bin]
//	std::vector<std::vector<int>> histogramRegion1;
//	std::vector<std::vector<int>> histogramRegion2;
//};

//! Calculates different metrics of histograms
class iAVRHistogramMetric: public iAVRMetrics
{
public:
	iAVRHistogramMetric(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	iAVRHistogram getHistogram(std::vector<double> values, int observations);
	iAVRHistogram getHistogram(std::vector<double> values, double min, double max, int observations);
	int determineNumberOfBins(int observations, int method);
	void setHistogramFeatureID(iAVRHistogram* histogram, int ID);
	

private:

	/**************************Histogram Calculations**********************************/
	int calculateSturgesRule(int observations);
	iAVRHistogram calculate1DHistogram(QString label, int bins, double min, double max, std::vector<double> values);
	std::vector<int> getObservationsInBin(iAVRHistogram* histogram);

	/**************************Descriptive Statistics**********************************/
	void calculateDescriptiveStatistics(std::vector<double> values, iAVRHistogram* histogram);
	double calculateMean(std::vector<double> values);
	double calculateMedian(std::vector<double> values);
	double calculateStandartDeviation(std::vector<double> values);
	double calculateSkewness(std::vector<double> values);
	double calculateKurtosis(std::vector<double> values);
	int calculateModality(std::vector<double> values);

	
};


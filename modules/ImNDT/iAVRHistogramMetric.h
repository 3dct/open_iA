// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRHistogram.h"
#include "iAVRMetrics.h"

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
	iAVRHistogramMetric(vtkTable* objectTable, std::vector<iAVROctree*> const & octrees);
	//! Returns the calculated histogram as iAVRHistogram class.
	//! The amount of bins are calculated based on observations and the min and max of the histogram are based on the min/max value in the given values.
	//! The returned Histogram is a 1D histogram and consists of underflow and overflow bins. The max value is in the overflow bin [max,inf), values below the min value are in the underflow bin.
	//! == Intervals are semi-open [a, b)
	iAVRHistogram getHistogram(std::vector<double> values, int observations);
	//! Returns the calculated histogram as iAVRHistogram class.
	//! The amount of bins are calculated based on observations and the given min and max values.
	//! The returned Histogram is a 1D histogram and consists of underflow and overflow bins. The max value is in the overflow bin [max,inf), values below the min value are in the underflow bin.
	//! == Intervals are semi-open [a, b)
	iAVRHistogram getHistogram(std::vector<double> values, double min, double max, int observations);
	//! Returns the amount of bins needed for a histogram based one maximum number of observations and an calculation choose with method
	//! Method 0: Sturge's Rule
	int determineNumberOfBins(int observations, int method);
	//! Sets the ID of the feature which is represented by this histogram
	void setHistogramFeatureID(iAVRHistogram* histogram, int ID);


private:

	/**************************Histogram Calculations**********************************/
	//! Calculates the number of needed bins through Sturge's Rule
	int calculateSturgesRule(int observations);
	iAVRHistogram calculate1DHistogram(QString label, int bins, double min, double max, std::vector<double> values);
	std::vector<int> getObservationsInBin(iAVRHistogram* histogram);
	
	/**************************Descriptive Statistics**********************************/
	//! Calculates the descriptive statistics for every histogram.
	//! Every calculated parameter is stored in the respective variable in the given histogram class
	void calculateDescriptiveStatistics(std::vector<double> values, iAVRHistogram* histogram);
};

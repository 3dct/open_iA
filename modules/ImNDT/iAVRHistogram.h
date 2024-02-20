// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning( disable : 4702)
#endif
#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/**************************Define histogram type**********************************/
using axes_regular = std::tuple<
	boost::histogram::axis::regular<>
>;
using regularStatic1DHistogram = boost::histogram::histogram<axes_regular>;
/*********************************************************************************/

struct HistogramParameters
{
	int featureID = -1;
	double bins = 0;
	double binWidth = 0;
	//Holds the observations for every [bin]
	std::vector<int> observations = std::vector<int>();
	double maxValue = 0;
	double minValue = 0;
	double mean = 0;
	double median = 0;
	double standartDeviation = 0;
	double skewness = 0;
	double kurtosis = 0;
	double modality = 0;
};

//! Stores Histogram values
class iAVRHistogram
{
public:
	iAVRHistogram();
	HistogramParameters m_histogramParameters;

	//! Returns a pointer to the boost::histogram structure
	regularStatic1DHistogram* getHistogram();

private:
	regularStatic1DHistogram m_histogram;

	void initializeDataStructure();
};

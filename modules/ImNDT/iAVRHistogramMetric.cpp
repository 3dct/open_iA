// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAVRHistogramMetric.h"

#include <algorithm>           // std::for_each
//#include <functional>          // std::ref (?)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning( disable : 4459 )
#endif
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

//#include <boost/format.hpp>    // only needed for printing
//#include <boost/histogram.hpp> // make_histogram, regular, weight, indexed


iAVRHistogramMetric::iAVRHistogramMetric(vtkTable* objectTable, std::vector<iAVROctree*> const & octrees)
	: iAVRMetrics(objectTable, octrees)
{
}

iAVRHistogram iAVRHistogramMetric::getHistogram(std::vector<double> values, int observations)
{
	auto bins = determineNumberOfBins(observations, 0);
	auto minMaxElem = std::minmax_element(values.begin(), values.end());
	return calculate1DHistogram(QString("Feature"), bins, *minMaxElem.first, *minMaxElem.second, values);
}

iAVRHistogram iAVRHistogramMetric::getHistogram(std::vector<double> values, double min, double max, int observations)
{
	auto bins = determineNumberOfBins(observations, 0);
	return calculate1DHistogram(QString("Feature"), bins, min, max, values);
}

int iAVRHistogramMetric::determineNumberOfBins(int observations, int method)
{
	Q_UNUSED(method);
	return calculateSturgesRule(observations);
}

void iAVRHistogramMetric::setHistogramFeatureID(iAVRHistogram* histogram, int ID)
{
	histogram->m_histogramParameters.featureID = ID;
	// The name of the feature is also set as axis metadata (boost)
	//histogram->getHistogram()->axis(0).metadata() = getFeatureName(ID).toStdString();
}

int iAVRHistogramMetric::calculateSturgesRule(int observations)
{
	double bins = 1 + 3.322 * log(observations);
	return round(bins);
}

iAVRHistogram iAVRHistogramMetric::calculate1DHistogram(QString label, int bins, double min, double max, std::vector<double> values)
{
	using namespace boost::histogram; // strip the boost::histogram prefix
	iAVRHistogram histogram = iAVRHistogram();

	/*
	  Create a 1d-histogram with a regular axis that has 6 equidistant bins on
	  the real line from -1.0 to 2.0, and label it as "x". A family of overloaded
	  factory functions called `make_histogram` makes creating histograms easy.

	  A regular axis is a sequence of semi-open bins --> [-1.0, 2).
	  Extra under- and overflow bins extend the axis by default (this can be turned off).

	  index    :      -1  |  0  |  1  |  2  |  3  |  4  |  5  |  6
	  bin edges:  -inf  -1.0  -0.5   0.0   0.5   1.0   1.5   2.0   inf
	*/
	*histogram.getHistogram() = make_histogram(axis::regular<>(bins, min, max, label.toStdString()));

	auto ind = indexed(*histogram.getHistogram(), coverage::inner);
	auto&& secondBin = ind.begin(); // starts at inner = 0

	//Fill the histogram with data
	std::for_each(values.begin(), values.end(), std::ref(*histogram.getHistogram()));

	//Fill parameters of histogramm
	histogram.m_histogramParameters.bins = histogram.getHistogram()->size();
	histogram.m_histogramParameters.binWidth = secondBin->bin().width();
	histogram.m_histogramParameters.minValue = min;
	histogram.m_histogramParameters.maxValue = max;
	histogram.m_histogramParameters.observations = getObservationsInBin(&histogram);
	calculateDescriptiveStatistics(values, &histogram);

	//LOG(lvlDebug, "1DHistogram calculated");

	//LOG(lvlImportant, QString("\n New Hist with:\n Bins: %1 \n Bin Width: %2 \n Min: %3 \n Max: %4 \n").arg(histogram.m_histogramParameters.bins).arg(histogram.m_histogramParameters.binWidth).arg(histogram.m_histogramParameters.minValue).arg(histogram.m_histogramParameters.maxValue));
	//LOG(lvlImportant, QString("\n Mean: %1 \n Median: %2 \n StandartDeviation: %3 \n kurtosis: %4 \n skewness: %5 \n").arg(histogram.m_histogramParameters.mean).arg(histogram.m_histogramParameters.median).arg(histogram.m_histogramParameters.standartDeviation).arg(histogram.m_histogramParameters.kurtosis).arg(histogram.m_histogramParameters.skewness));

	return histogram;
}

std::vector<int> iAVRHistogramMetric::getObservationsInBin(iAVRHistogram* histogram)
{
	using namespace boost::histogram; // strip the boost::histogram prefix

	// Create Vector
	std::vector<int> vec;

	//LOG(lvlImportant, QString("Copy Hist:\n Bins: %1 \n Bin Width: %2 \n Min: %3 \n").arg(histogram->m_histogramParameters.bins).arg(histogram->m_histogramParameters.binWidth).arg(histogram->m_histogramParameters.minValue));

	for (auto&& x : indexed(*histogram->getHistogram(), coverage::all)) {
		vec.push_back(*x);
	}

	return vec;
}

void iAVRHistogramMetric::calculateDescriptiveStatistics(std::vector<double> values, iAVRHistogram* histogram)
{
	using namespace boost::accumulators;
	accumulator_set<double, stats<tag::mean, tag::median, tag::variance, tag::kurtosis, tag::skewness >> acc;

	//Fill the accumulator with data
	std::for_each(values.begin(), values.end(), std::ref(acc));

	histogram->m_histogramParameters.mean = mean(acc);
	histogram->m_histogramParameters.median = median(acc);
	histogram->m_histogramParameters.standartDeviation = sqrt(variance(acc));
	histogram->m_histogramParameters.kurtosis = kurtosis(acc);
	histogram->m_histogramParameters.skewness = skewness(acc);
}

#include "ThresholdCalcHelper.h"
#include <numeric>
#include "ThresholdDefinitions.h"
#include <algorithm>
#include <stdexcept>

#include "ThresAlgo.h"

double ThresholdCalcHelper::findMaxPeak(std::vector<double>& v_ind) const
{
	std::sort(v_ind.begin(), v_ind.end(), algorithm::greaterThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());

	if (peak == v_ind.end()) {
		--peak;
	}

	return *peak;
}

double ThresholdCalcHelper::findMinPeak(std::vector<double>& v_ind) const {
	std::sort(v_ind.begin(), v_ind.end(), algorithm::smallerThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());

	if (peak == v_ind.end()) {
		--peak;
	}

	return *peak;
}

double ThresholdCalcHelper::vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd)
{
	if (startInd >= vec.size() || endInd >= vec.size()) throw new std::invalid_argument("test");
	double tmp = 0.0f;

	size_t i = startInd;
	while (i <= endInd) {

		tmp += vec[i];
		++i;
	}

	return tmp;
}

threshold_defs::ThresIndx ThresholdCalcHelper::findIndex(const std::vector<double>& vec, double cmpVal) const
{

	threshold_defs::ThresIndx thrInd;
	long ind = 0;
	thrInd.value = cmpVal;

	for (const double& el : vec) {
		bool isEqual = algorithm::compareDouble(el, cmpVal);
		if (isEqual) {
			thrInd.thrIndx = ind;
			break;
		}
		else {
			thrInd.thrIndx = -1;
		}

		ind++;
	}

	return thrInd;

}

threshold_defs::ThresMinMax ThresholdCalcHelper::calculateMinMax(const threshold_defs::ParametersRanges& inRanges) const
{
	double y_min = 0;
	double y_max = 0;
	std::vector<double> yRange = inRanges.getYRange();
	std::vector<double> xRange = inRanges.getXRange();

	y_max = this->findMaxPeak(yRange);
	y_min = this->findMinPeak(yRange);

	//threshold_defs::ThresIndx indMax;
	const auto indMax = this->findIndex(yRange, y_max);
	const auto indMin = this->findIndex(yRange, y_min);
	double x_max = xRange[indMax.thrIndx];
	double x_min = xRange[indMin.thrIndx];	
	
	threshold_defs::ThresMinMax thrMinMax;

	thrMinMax.maxThresholdY = y_max;
	thrMinMax.minThresholdY = y_min;
	thrMinMax.minX = x_min;
	thrMinMax.maxX = x_max;

	return thrMinMax; 
}
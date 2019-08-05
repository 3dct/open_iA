#include "ThresholdCalcHelper.h"
#include <numeric>
#include "ThresholdDefinitions.h"
#include <algorithm>
#include <stdexcept>

#include "ThresAlgo.h"

double ThresholdCalcHelper::findMaxPeak(std::vector<double>& v_ind)
{
	std::sort(v_ind.begin(), v_ind.end(), algorithm::greaterThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());

	if (peak == v_ind.end()) {
		--peak;
	}

	return *peak;
}

double ThresholdCalcHelper::findMinPeak(std::vector<double>& v_ind) {
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

ThresIndx ThresholdCalcHelper::findIndex(const std::vector<double>& vec, double cmpVal)
{
	ThresIndx thrInd;
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

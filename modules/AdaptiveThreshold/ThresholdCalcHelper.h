#pragma once

#include <vector>

class ThresholdCalcHelper
{
public:
	//ThresholdCalcHelper(); 


	double findMaxPeak(std::vector<double>& v_ind/*, unsigned int toleranceVal*/);
	double findMinPeak(std::vector<double>& v_ind);


	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);
};


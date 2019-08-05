#pragma once

#include <vector>
#include "ThresholdDefinitions.h"
//class threshold_defs::ThresIndx;

class ThresholdCalcHelper
{
public:
	//ThresholdCalcHelper(); 


	double findMaxPeak(std::vector<double>& v_ind);
	double findMinPeak(std::vector<double>& v_ind);
	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);
	threshold_defs::ThresIndx findIndex(const std::vector<double>& vec, double cmpVal);

private:
	
};


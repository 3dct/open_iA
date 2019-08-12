#pragma once

#include <vector>
#include "ThresholdDefinitions.h"
//class threshold_defs::ThresIndx;

class ThresholdCalcHelper
{
public:
	//ThresholdCalcHelper(); 
	double findMaxPeak(std::vector<double>& v_ind) const;
	double findMinPeak(std::vector<double>& v_ind) const;
	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);
	threshold_defs::ThresIndx findIndex(const std::vector<double>& vec, double cmpVal) const;
		

	threshold_defs::ThresMinMax calculateMinMax(const threshold_defs::ParametersRanges& inRanges) const;

	//iso 50 grauwert between material peak and air peak
	void determinIso50(const threshold_defs::ParametersRanges& inRanges, threshold_defs::ThresMinMax &inVals);

private:
	
};


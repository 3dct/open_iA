#pragma once

#include <vector>
class ThresIndx; 

class ThresholdCalcHelper
{
public:
	//ThresholdCalcHelper(); 


	double findMaxPeak(std::vector<double>& v_ind);
	double findMinPeak(std::vector<double>& v_ind);
	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);
	ThresIndx findIndex(const std::vector<double>& vec, double cmpVal);

private:
	

};


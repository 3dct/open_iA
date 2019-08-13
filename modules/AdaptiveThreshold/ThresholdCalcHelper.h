#pragma once

#include <vector>
#include "ThresholdDefinitions.h"
//class threshold_defs::ThresIndx;
class QPointF; 

class ThresholdCalcHelper
{
public:
	//ThresholdCalcHelper(); 
	double findMaxPeak(std::vector<double>& v_ind) const;
	double findMinPeak(std::vector<double>& v_ind) const;
	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);

	//calculate the intersection between points and a threshold  
	std::vector<QPointF> determineIntersections(const threshold_defs::ParametersRanges in_range, double vals); 

	QPointF calculateIntersection(const threshold_defs::ParametersRanges& ranges,
		const  QPointF& pt_1, const QPointF& pt2);

	threshold_defs::ThresIndx findIndex(const std::vector<double>& vec, double cmpVal) const;
		

	threshold_defs::ThresMinMax calculateMinMax(const threshold_defs::ParametersRanges& inRanges) const;

	//iso 50 gray value between material peak and air peak
	/*max peak detection
	*air peak already detected 
	*/
	void determinIso50(const threshold_defs::ParametersRanges& inRanges, threshold_defs::ThresMinMax &inVals);

};


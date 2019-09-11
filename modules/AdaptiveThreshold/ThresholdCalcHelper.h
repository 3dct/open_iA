#pragma once

#include <vector>
#include "ThresholdDefinitions.h"
#include <algorithm>
#include <QVector>
#include <stdexcept>
#include "ThresAlgo.h"

class QPointF;

/*
bool operator<(QPointF const& p1, QPointF const& p2)
{
	return p1.x() < p2.x();
}
*/

class ComparablePointFX
{
public:
	bool operator()(const QPointF& p1, const QPointF& p2) {
		return p1.x() < p2.x();
	}
};

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
		


	/*
	thrMinMax.FreqPeakLokalMaxY(y_max);
	thrMinMax.FreqPeakMinY(y_min);
	thrMinMax.PeakMinXThreshold(x_min);
	thrMinMax.LokalMaxPeakThreshold_X(x_max);
	*/
	threshold_defs::ThresMinMax calculateLocalePeaks(const threshold_defs::ParametersRanges& inRanges) const;

	//iso 50 gray value between material peak and air peak
	/*max peak detection
	*air peak already detected 
	*/
	void determinIso50andGlobalMax(const threshold_defs::ParametersRanges& inRanges, threshold_defs::ThresMinMax &inVals);

	
	void getFirstElemInRange(const QVector <QPointF>& in, float xmin, float xmax, QPointF* result);


	inline void testSortPointsByIdx() {
		QVector<QPointF> vec; 
		QPointF pt1(0.0f, 1.0f);
		QPointF pt2(4.1f, -3.4f); 
		QPointF pt3(10.0f, 14.3f);
		QPointF pt4(-10.0f, 2.0f);
		QPointF pt5(3.0f, -19.0f);
		QPointF pt6(-101.0f, 201.0f); 
		vec.push_back(pt1);
		vec.push_back(pt2);
		vec.push_back(pt3);
		vec.push_back(pt4);
		vec.push_back(pt5);
		vec.push_back(pt6); 

		//this->sortPointsByX(vec); 

		QPointF tmp(-99, -99); 
		this->getFirstElemInRange(vec, 100, 200, &tmp);

	}
	
	inline void sortPointsByX(QVector<QPointF>& vec) {
		//std::sort(vec.begin(), vec.end()/*, &greaterThan/ *<QPointF>()*/);
		std::sort(vec.begin(), vec.end(), ComparablePointFX());
	}


	void PeakgreyThresholdNormalization(threshold_defs::ParametersRanges& ranges, double greyThrPeakAir, double greyThrPeakMax); 

private: 

	//checks if a point is between min and max
	inline bool checkInRange(const QPointF& pt, float min, float max) {
		float xval = (float)  pt.x(); 
		bool isInRangeMin = false; 
		bool isInRangeMax = false; 
		
		if (!(min < max))
			throw std::invalid_argument("error comparing values");

		isInRangeMin = (algorithm::compareDouble(min, xval)) || (min < xval) ;
		isInRangeMax = (algorithm::compareDouble(max, xval)) || (xval < max) ;
		
		return (isInRangeMax && isInRangeMin); 
		
	}

	
};

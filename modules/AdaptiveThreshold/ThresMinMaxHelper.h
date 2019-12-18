#pragma once
#include "ThresholdDefinitions.h"
class QTextEdit;
//algoritm description see paper::: 
/*
based on precalculated values:
lokal min max, iso 50, and intersection of fair/2 with greyscale curve
stored in ThresMinMax

Iryna Tretiak, Robert A. Smith,
A parametric study of segmentation thresholds for X-ray CT porosity characterisation in composite materials,
Composites Part A: Applied Science and Manufacturing,Volume 123,2019,Pages 10-24,
https://doi.org/10.1016/j.compositesa.2019.04.029
*/

namespace threshold_defs {

	class ThresMinMaxHelper
	{
	public:

		/*
		*compare fmin with fair/2
		if fmin > fair/2 take first minimum (gmin, fmin) as greythreshold, else take the next crossing of fair/2 or 50 %, whichever is lowest
		*/
		QPointF determineThresholdResultsPointXY(const ThresMinMax& results, QTextEdit *elem);

	private:
		QPointF getIntersectionPoint(const ThresMinMax& results);

		//fmin > fair /2
		bool compareFminWithAirPeak(const double fmin, const ThresMinMax& results);
		QPointF getLokalMininum(const ThresMinMax& results);

	};
};

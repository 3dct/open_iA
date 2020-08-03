/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAThresholdDefinitions.h"

#include <QVector>

#include <algorithm>
#include <stdexcept>
#include <vector>

class QPointF;

class iAThresholdCalcHelper
{
public:
	double findMaxPeak(std::vector<double>& v_ind) const;
	double findMinPeak(std::vector<double>& v_ind) const;
	double vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);

	//calculate the intersection between points and a threshold
	std::vector<QPointF> determineIntersections(const threshold_defs::iAParametersRanges in_range, double vals);

	QPointF calculateIntersection(const threshold_defs::iAParametersRanges& ranges,
		const  QPointF& pt_1, const QPointF& pt2);

	threshold_defs::iAThresIndx findIndex(const std::vector<double>& vec, double cmpVal) const;

	threshold_defs::iAThresMinMax calculateLocalePeaks(const threshold_defs::iAParametersRanges& inRanges) const;

	//iso 50 gray value between material peak and air peak
	/*max peak detection
	*air peak already detected
	*/
	void determinIso50andGlobalMax(const threshold_defs::iAParametersRanges& inRanges, threshold_defs::iAThresMinMax &inVals);

	void getFirstElemInRange(const QVector <QPointF>& in, float xmin, float xmax, QPointF* result);

	inline void testSortPointsByIdx()
	{
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

	inline void sortPointsByX(QVector<QPointF>& vec)
	{
		std::sort(vec.begin(), vec.end(), [](const QPointF & p1, const QPointF & p2)
		{
			return p1.x() < p2.x();
		});
	}

	void PeakgreyThresholdNormalization(threshold_defs::iAParametersRanges& ranges, double greyThrPeakAir, double greyThrPeakMax);

private:

	//checks if a point is between min and max
	bool checkInRange(const QPointF& pt, float min, float max);
};

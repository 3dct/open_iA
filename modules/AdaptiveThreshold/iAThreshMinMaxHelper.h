/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

class QTextEdit;

namespace threshold_defs {

	class iAThreshMinMaxHelper
	{
	public:

		/*
		*compare fmin with fair/2
		if fmin > fair/2 take first minimum (gmin, fmin) as greythreshold, else take the next crossing of fair/2 or 50 %, whichever is lowest
		*/
		QPointF determineThresholdResultsPointXY(const iAThresMinMax& results, QTextEdit *elem);

	private:
		QPointF getIntersectionPoint(const iAThresMinMax& results);

		//fmin > fair /2
		bool compareFminWithAirPeak(const iAThresMinMax& results);
		QPointF getLokalMininum(const iAThresMinMax& results);

	};
};

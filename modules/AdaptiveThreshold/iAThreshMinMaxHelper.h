// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

		QPointF getLokalMininum(const iAThresMinMax& results);

	};
};

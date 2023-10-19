// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAThresholdDefinitions.h"

enum LineVisOption
{
	horizontally,
	vertically,
	horizontal_xy
};

class QPointF;
class QColor;

class QLineSeries;
class QXYSeries;

//! @{ Create a line series
QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges);
QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);
//! @}

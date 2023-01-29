// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAThresholdDefinitions.h"

#include <vector>

enum LineVisOption
{
	horizontally,
	vertically,
	horizontal_xy
};

class QPointF;
class QColor;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
namespace QtCharts
{
#endif
	class QLineSeries;
	class QXYSeries;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
}
#endif

//! @{ Create a line series
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QtCharts::QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges);
QtCharts::QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);
#else
QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges);
QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);
#endif
//! @}

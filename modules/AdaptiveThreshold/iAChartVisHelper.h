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

#include <vector>

enum LineVisOption
{
	horizontally,
	vertically,
	horizontal_xy
};

class QPointF;
class QColor;

namespace QtCharts
{
	class QLineSeries;
	class QScatterSeries;
	class QXYSeries;
}

//! @{ Create a line series
QtCharts::QLineSeries* createLineSeries(const threshold_defs::iAParametersRanges& ranges);
QtCharts::QLineSeries* createLineSeries(const std::vector<double>& vec_x,
	const std::vector<double>& vec_y);
QtCharts::QLineSeries* createLineSeries(const QPointF& pt_1, const QPointF& pt_2, LineVisOption option);
//! @}

//! @{ Create a scatter series
QtCharts::QScatterSeries* createScatterSeries(const threshold_defs::iAParametersRanges& ranges);
QtCharts::QScatterSeries* createScatterSeries(const std::vector<double>& vec_x,
	const std::vector<double>& vec_y);
QtCharts::QScatterSeries *createScatterSeries(const std::vector<QPointF> pts, double *pt_size);
QtCharts::QScatterSeries *createScatterSeries(const QPointF& pt, double pt_size, const QColor* color);
//! @}
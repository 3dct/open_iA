// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

class iAColoredPolyObjectVis;
class iAPolyObjectVisActor;
class iAChartWidget;
class iAFixedAspectWidget;
class iASignallingWidget;
class iAStackedBarChart;
class iAQVTKWidget;
class QWidget;

//! UI elements for each result
class iAFiberResultUIData
{
public:
	iAQVTKWidget* vtkWidget = nullptr;
	std::shared_ptr<iAColoredPolyObjectVis> mini3DVis;
	std::shared_ptr<iAPolyObjectVisActor> mini3DActor;
	std::shared_ptr<iAColoredPolyObjectVis> main3DVis;
	std::shared_ptr<iAPolyObjectVisActor> main3DActor;
	iAChartWidget* histoChart;
	iAStackedBarChart* stackedBars;
	iAFixedAspectWidget* previewWidget = nullptr;
	iASignallingWidget* nameActions;
	QWidget* topFiller, * bottomFiller;
	//! index where the plots for this result start
	size_t startPlotIdx;
};

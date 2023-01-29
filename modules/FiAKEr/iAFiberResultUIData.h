// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSharedPointer>

class iA3DColoredPolyObjectVis;
class iA3DPolyObjectActor;
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
	QSharedPointer<iA3DColoredPolyObjectVis> mini3DVis;
	QSharedPointer<iA3DPolyObjectActor> mini3DActor;
	QSharedPointer<iA3DColoredPolyObjectVis> main3DVis;
	QSharedPointer<iA3DPolyObjectActor> main3DActor;
	iAChartWidget* histoChart;
	iAStackedBarChart* stackedBars;
	iAFixedAspectWidget* previewWidget = nullptr;
	iASignallingWidget* nameActions;
	QWidget* topFiller, * bottomFiller;
	//! index where the plots for this result start
	size_t startPlotIdx;
};

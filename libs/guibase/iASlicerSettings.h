// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "defines.h"   // for DefaultMagicLensSize

#include <QString>

//! Settings applicable to a single slicer window.
class iASingleSlicerSettings
{
public:
	bool LinearInterpolation,
		ShowIsoLines,
		ShowPosition,
		ShowAxesCaption,
		ShowTooltip,
		AdjustWindowLevelEnabled;
	double MinIsoValue, MaxIsoValue;
	int NumberOfIsoLines,
		ToolTipFontSize,
		MagicLensSize,          //!< size (width & height) of the 2D magic lens (in pixels / pixel-equivalent units considering scaling)
		MagicLensFrameWidth;    //!< width of the frame of the 2D magic lens
	QString CursorMode;
	QColor backgroundColor;

	iASingleSlicerSettings() :
		LinearInterpolation(true),
		ShowIsoLines(false),
		ShowPosition(false),
		ShowAxesCaption(false),
		ShowTooltip(true),
		AdjustWindowLevelEnabled(false),
		MinIsoValue(20000),
		MaxIsoValue(40000),
		NumberOfIsoLines(5),
		ToolTipFontSize(12),
		MagicLensSize(DefaultMagicLensSize),
		MagicLensFrameWidth(3),
		CursorMode(QString("Crosshair default")),
		backgroundColor()	// invalid
	{}
};

//! Settings for slicer windows and their interaction among each other and with other windows.
class iASlicerSettings
{
public:
	bool InteractorsEnabled,
		LinkViews,
		LinkMDIs;
	int SnakeSlices;
	iASingleSlicerSettings SingleSlicer;
	QString BackgroundColor[3]; // background color per slicer mode

	iASlicerSettings() :
		InteractorsEnabled(true),
		LinkViews(false),
		LinkMDIs(false),
		SnakeSlices(100)
	{}

};

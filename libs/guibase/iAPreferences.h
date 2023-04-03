// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "defines.h" // for DefaultMagicLensSize

//! Collection of generic open_iA program preferences.
class iAPreferences
{
	// TODO: currently, all of these are stored per MdiChild; some of them should be global though
public:
	int HistogramBins,
		PositionMarkerSize,
		MagicLensSize,
		MagicLensFrameWidth,
		FontSize,
		LimitForAuto3DRender;
	bool ResultInNewWindow,
		PrintParameters,
		HistogramLogarithmicYAxis;
		//LogToFile;
	iAPreferences():
		HistogramBins(2048),
		PositionMarkerSize(3),
		MagicLensSize(DefaultMagicLensSize),
		MagicLensFrameWidth(3),
		FontSize(8),
		LimitForAuto3DRender(2'000),
		ResultInNewWindow(true),
		PrintParameters(true),
		HistogramLogarithmicYAxis(false)
	{}
};

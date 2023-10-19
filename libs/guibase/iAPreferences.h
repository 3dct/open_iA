// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAMagicLensConstants.h" // for DefaultMagicLensSize

//! Collection of generic open_iA program preferences.
class iAPreferences
{
	// TODO: currently, all of these are stored per MdiChild; some of them should be global though
public:
	int FontSize,
		PositionMarkerSize,
		LimitForAuto3DRender;
	bool ResultInNewWindow,
		PrintParameters;
		//LogToFile;
	iAPreferences():
		FontSize(8),
		PositionMarkerSize(3),
		LimitForAuto3DRender(2'000),
		ResultInNewWindow(true),
		PrintParameters(true)
	{}
};

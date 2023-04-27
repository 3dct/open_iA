// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//! Settings for slicer windows and their interaction among each other and with other windows.

// addAttr(attr, "Interaction Enabled", iAValueType::Boolean, true);
// addAttr(attr, "Link Views", iAValueType::Boolean, false);
// addAttr(attr, "Link MDIs", iAValueType::Boolean, false);
// addAttr(attr, "Snake slices", iAValueType::Discrete, 100);

class iASlicerSettings
{
public:
	bool InteractorsEnabled,
		LinkViews,
		LinkMDIs;
	int SnakeSlices;
	iASlicerSettings() :
		InteractorsEnabled(true),
		LinkViews(false),
		LinkMDIs(false),
		SnakeSlices(100)
	{}
};
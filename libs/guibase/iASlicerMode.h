// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//! Index of the coordinate axes, to prevent "magic numbers" in code
enum iAAxisIndex
{
	X,
	Y,
	Z,
	AxisCount
};

//! Constants for the three axis-aligned slicer modes.
enum iASlicerMode
{
	YZ,  // slice along x-Axis
	XZ,  // slice along y-Axis
	XY,  // slice along z-Axis
	SlicerCount
};

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QString>

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

// Helper functions for axes and slicer modes

//! Get the name of the given axis
//! @param axis the index of the axis (see iAAxisIndex)
iAguibase_API QString axisName(int axis);

//! Convert an axis name to the iAAxisIndex enum
iAguibase_API iAAxisIndex nameToAxis(QString const& name);

//! Get the "name" of the given slicer mode (i.e. the slicer plane, "XY" for iASlicerMode XY).
iAguibase_API QString slicerModeString(int mode);

//! Map the index of an axis of the slicer to the index of the corresponding global axis.
//! @param mode the slicer mode, @see iASlicerMode
//! @param index the slicer axis index (x=0, y=1, z=2), @see iAAxisIndex
//! @return the global axis index; for values of index = 0,1,2 it returns:
//! - 1, 2, 0 for mode=YZ
//! - 0, 2, 1 for mode=XZ
//! - 0, 1, 2 for mode=XY
iAguibase_API int mapSliceToGlobalAxis(int mode, int index);

// ! Map the index of a global axis to the index of the corresponding axis of the slicer.
// ! @param mode the slicer mode, @see iASlicerMode
// ! @param index the slicer axis index (x=0, y=1, z=2), @see iAAxisIndex
// ! @return the slicer axis index; for values of index = 0,1,2 it returns:
// ! - 2, 0, 1 for YZ
// ! - 0, 2, 1 for XZ
// ! - 0, 1, 2 for XY
// Currently not used...
// iAguibase_API int mapGlobalToSliceAxis(int mode, int index);

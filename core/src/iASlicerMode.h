/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

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

// Helper functions for axes and slicer modes - defined in iASlicer.cpp!

//! Get the name of the given axis
//! @param axis the index of the axis (see iAAxisIndex)
open_iA_Core_API QString axisName(int axis);

//! Get the "name" of the given slicer mode (i.e. the slicer plane, "XY" for iASlicerMode XY).
open_iA_Core_API QString slicerModeString(int mode);

//! Map the index of an axis of the slicer to the index of the corresponding global axis.
//! @param mode the slicer mode, @see iASlicerMode
//! @param index the slicer axis index (x=0, y=1, z=2), @see iAAxisIndex
//! @return the global axis index; for values of index = 0,1,2 it returns:
//! - 1, 2, 0 for mode=YZ
//! - 0, 2, 1 for mode=XZ
//! - 0, 1, 2 for mode=XY
open_iA_Core_API int mapSliceToGlobalAxis(int mode, int index);

// ! Map the index of a global axis to the index of the corresponding axis of the slicer.
// ! @param mode the slicer mode, @see iASlicerMode
// ! @param index the slicer axis index (x=0, y=1, z=2), @see iAAxisIndex
// ! @return the slicer axis index; for values of index = 0,1,2 it returns:
// ! - 2, 0, 1 for YZ
// ! - 0, 2, 1 for XZ
// ! - 0, 1, 2 for XY
// Currently not used...
// open_iA_Core_API int mapGlobalToSliceAxis(int mode, int index);

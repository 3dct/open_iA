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

enum iASlicerMode
{
	YZ,  // slice along x-Axis
	XZ,  // slice along y-Axis
	XY,  // slice along z-Axis
	SlicerCount
};

//! retrieve the "name" of the given slicer mode (i.e. the slicer plane, "XY" for iASlicerMode XY)
open_iA_Core_API QString slicerModeString(int mode);

//! return the name of axis along which the given slicer mode cuts (i.e. "Z" for slice mode "XY")
open_iA_Core_API QString sliceAxis(int mode);

//! get the index of the dimension along which the given slicer mode cuts (0 for YZ, 1 for XZ, 2 for XY)
open_iA_Core_API int slicerDimension(int mode);

//! get two axes of slicer - 0, 1 for XY; 0, 2 for XZ; 1, 2 for YZ
open_iA_Core_API int sliceAxis(int axis, int index);

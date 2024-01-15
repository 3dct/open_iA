// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASlicerMode.h"

QString axisName(int axis)
{
	switch (axis)
	{
	case iAAxisIndex::X: return "X";
	case iAAxisIndex::Y: return "Y";
	case iAAxisIndex::Z: return "Z";
	default: return "?";
	}
}

QString slicerModeString(int mode)
{
	return axisName(mapSliceToGlobalAxis(mode, iAAxisIndex::X)) + axisName(mapSliceToGlobalAxis(mode, iAAxisIndex::Y));
}

namespace
{
	static const int SliceToGlobalAxisMapping[3][3] =
	{
		//            YZ              XZ              XY
		{ iAAxisIndex::Y, iAAxisIndex::X, iAAxisIndex::X },  // x
		{ iAAxisIndex::Z, iAAxisIndex::Z, iAAxisIndex::Y },  // y
		{ iAAxisIndex::X, iAAxisIndex::Y, iAAxisIndex::Z }   // z
	};
	/*
	static const int GlobalToSliceAxisMapping[3][3] =
	{   //            YZ              XZ              XY
		{ iAAxisIndex::Z, iAAxisIndex::X, iAAxisIndex::X }, // x
		{ iAAxisIndex::X, iAAxisIndex::Z, iAAxisIndex::Y }, // y
		{ iAAxisIndex::Y, iAAxisIndex::Y, iAAxisIndex::Z }  // z
	};
	*/
}

int mapSliceToGlobalAxis(int mode, int index)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	assert(0 <= index && index < iAAxisIndex::AxisCount);
	return SliceToGlobalAxisMapping[index][mode];
}
/*
int mapGlobalToSliceAxis(int mode, int index)
{
	return GlobalToSliceAxisMapping[index][mode];
}
*/

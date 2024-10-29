// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASlicerMode.h"

#include <array>

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

iAAxisIndex nameToAxis(QString const& name)
{
	if (name == "X" || name == "x") { return iAAxisIndex::X; }
	if (name == "Y" || name == "y") { return iAAxisIndex::Y; }
	if (name == "Z" || name == "z") { return iAAxisIndex::Z; }
	else                            { return iAAxisIndex::AxisCount; }
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
	static const int GlobalToSliceAxisMapping[3][3] =
	{   //            YZ              XZ              XY
		{ iAAxisIndex::Z, iAAxisIndex::X, iAAxisIndex::X }, // x
		{ iAAxisIndex::X, iAAxisIndex::Z, iAAxisIndex::Y }, // y
		{ iAAxisIndex::Y, iAAxisIndex::Y, iAAxisIndex::Z }  // z
	};
}

int mapSliceToGlobalAxis(int mode, int localAxis)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	assert(0 <= localAxis && localAxis < iAAxisIndex::AxisCount);
	return SliceToGlobalAxisMapping[localAxis][mode];
}

int mapGlobalToSliceAxis(int mode, int globalAxis)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	assert(0 <= globalAxis && globalAxis < iAAxisIndex::AxisCount);
	return GlobalToSliceAxisMapping[globalAxis][mode];
}

int& axisColorMode()
{
	static int colorMode = 0;
	return colorMode;
}

QColor axisColor(int mode)
{   // see e.g. https://stackoverflow.com/questions/15810171 for why a "flat" list is initialized here:
	static std::array<std::array<QColor, 3>, 2> colors{
		 QColor(255, 0 , 0), QColor(0, 255, 0), QColor(0, 0, 255) ,
		 QColor(217, 95, 2), QColor(27, 158, 119), QColor(117, 112, 179)
	};
	return colors[axisColorMode()][mode];
}

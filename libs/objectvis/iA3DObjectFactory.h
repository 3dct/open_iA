// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAobjectvis_export.h"

#include <QColor>
#include <QMap>
#include <QSharedPointer>

#include <map>
#include <vector>

#include "iAVec3.h"

class iA3DObjectVis;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkTable;

iAobjectvis_API QSharedPointer<iA3DObjectVis> create3DObjectVis(int visualization, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides = 12, size_t segmentSkip = 1,
	vtkColorTransferFunction* ctf = nullptr, vtkPiecewiseFunction* otf = nullptr, double const* bounds = nullptr);

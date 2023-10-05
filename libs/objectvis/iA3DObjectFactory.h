// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include "iAObjectType.h"

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

//! Factory function for creating a fitting iA3DObjectVis-derived class for the given visualization type
//! @param visualization the type of visualization to create; see iACsvConfig::VisualizationType
//! @param table the table (one row per object to visualize)
//! @param columnMapping mapping of columns (see the respective visualization classes which mappings are required)
//! @param color the default color of the objects
//! @param curvedFiberInfo optional (if non-empty vector) information on curved fiber objects
//! @param numberOfCylinderSides quality parameter for cylinder visualization (lower values lead to faster
//!        rendering but also less quality; minimum value that makes sense is 3 (then you get a triangular tube)
//! @param segmentSkip quality parameter for cylinder visualization (how many segments for curved fibers are
//!        skipped; higher values lead to faster rendering but also less accurate representation of curved fibers
//!        the default value of 1 means don't skip; larger values skip curved fiber segments
//! @param ctf color transfer function; only used for labeled volume visualization
//! @param otf opacity transfer function; only used for labeled volume visualization
//! @param bounds optional bounds of the object visualization
iAobjectvis_API QSharedPointer<iA3DObjectVis> create3DObjectVis(iAObjectVisType visualization, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides = 12, size_t segmentSkip = 1,
	vtkColorTransferFunction* ctf = nullptr, vtkPiecewiseFunction* otf = nullptr, double const* bounds = nullptr);

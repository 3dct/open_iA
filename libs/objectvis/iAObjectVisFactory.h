// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include <QColor>

#include <memory>

class iAObjectsData;
class iAColoredPolyObjectVis;

//! Factory function for creating a fitting iAObjectVis-derived class for the given visualization type
//! @param data the data of the objects
//! @param color the default color of the objects
//! @param numberOfCylinderSides quality parameter for cylinder visualization (lower values lead to faster
//!        rendering but also less quality; minimum value that makes sense is 3 (then you get a triangular tube)
//! @param segmentSkip quality parameter for cylinder visualization (how many segments for curved fibers are
//!        skipped; higher values lead to faster rendering but also less accurate representation of curved fibers
//!        the default value of 1 means don't skip; larger values skip curved fiber segments
iAobjectvis_API std::shared_ptr<iAColoredPolyObjectVis> createObjectVis(iAObjectsData const* data,
	QColor const& color, int numberOfCylinderSides, size_t segmentSkip);

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// TODO: re-use in FIAKER/FiberSA(/DreamCaster?)
#include "iabase_export.h"

#include <iAVec3.h>

#include <QString>

#include <array>

//! An axis-aligned bounding box.
//!
//! Provides convenience functions for creating a bounding box (adding a single point, merging boxes),
//! as well as for testing containment/intersection.
class iAbase_API iAAABB
{
public:
	iAAABB();
	iAAABB(double const* b);
	//! add a point that should fit into the bounding box; if the current box does not contain this point, it is enlarged
	void addPointToBox(iAVec3d const& pt);
	//! merge another bounding box to this one, enlarging it if necessary
	void merge(iAAABB const& other);
	//! return true if the given point is contained within the bounding box
	bool contains(iAVec3d const& pt) const;
	//! return true if the given other bounding box has intersecting space with this
	bool intersects(iAAABB const& other) const;
	iAVec3d const& minCorner() const;
	iAVec3d const& maxCorner() const;

private:
	std::array<iAVec3d, 2> box;
};

//! Get a human-readable string representation of an iAAABB (axis-aligned bounding box)
iAbase_API QString toStr(iAAABB const& box);

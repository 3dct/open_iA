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
	//! construct an "empty" bounding box, ready to be adjusted to new points via addPointToBox
	//! All 3 maximum coordinates are set to the minimum possible double value,
	//! all 3 minimum coordinates are set to the maximum possible double value.
	iAAABB();
	//! construct bounding box from 6 double values (xmin, xmax, ymin, ymax, zmin, zmax)
	explicit iAAABB(double const b[6]);
	//! construct bounding box from 6 double values
	explicit iAAABB(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);
	//! add a point that should fit into the bounding box; if the current box does not contain this point, it is enlarged
	void addPointToBox(iAVec3d const& pt);
	//! merge another bounding box to this one, enlarging it if necessary
	void merge(iAAABB const& other);
	//! return true if the given point is contained within the bounding box
	bool contains(iAVec3d const& pt) const;
	//! return true if the given other bounding box has intersecting space with this
	bool intersects(iAAABB const& other) const;
	//! Retrieve the minimum of each of the 3 coordinates as point vector
	iAVec3d const& minCorner() const;
	//! Retrieve the maximum of each of the 3 coordinates as point vector
	iAVec3d const& maxCorner() const;

private:
	std::array<iAVec3d, 2> box;
};

//! Get a human-readable string representation of an iAAABB (axis-aligned bounding box)
iAbase_API QString toStr(iAAABB const& box);

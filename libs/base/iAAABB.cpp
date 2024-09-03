// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAABB.h"

#include <limits>

iAAABB::iAAABB()
{
	box[0].fill(std::numeric_limits<double>::max());
	box[1].fill(std::numeric_limits<double>::lowest());
}

iAAABB::iAAABB(double const b[6]) : box{ iAVec3d(b[0], b[2], b[4]), iAVec3d(b[1], b[3], b[5]) }
{}

iAAABB::iAAABB(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
	: box{ iAVec3d(xmin, ymin, zmin), iAVec3d(xmax, ymax, zmax) }
{}

void iAAABB::addPointToBox(iAVec3d const& pt)
{
	for (int i = 0; i < 3; ++i)
	{
		box[0][i] = std::min(box[0][i], pt[i]);
		box[1][i] = std::max(box[1][i], pt[i]);
	}
}

void iAAABB::merge(iAAABB const& other)
{
	addPointToBox(other.minCorner());
	addPointToBox(other.maxCorner());
}

bool iAAABB::contains(iAVec3d const& pt) const
{
	return
		minCorner().x() <= pt.x() && pt.x() <= maxCorner().x() &&
		minCorner().y() <= pt.y() && pt.y() <= maxCorner().y() &&
		minCorner().z() <= pt.z() && pt.z() <= maxCorner().z();
}

bool iAAABB::intersects(iAAABB const& other) const
{
	return
		maxCorner().x() > other.minCorner().x() && other.maxCorner().x() > minCorner().x() &&
		maxCorner().y() > other.minCorner().y() && other.maxCorner().y() > minCorner().y() &&
		maxCorner().z() > other.minCorner().z() && other.maxCorner().z() > minCorner().z();
}

iAVec3d const& iAAABB::minCorner() const
{
	return box[0];
}

iAVec3d const& iAAABB::maxCorner() const
{
	return box[1];
}

bool operator!=(iAAABB const& a, iAAABB const& b)
{
	return a.minCorner() != b.minCorner() || a.maxCorner() != b.maxCorner();
}

QString toStr(iAAABB const& box)
{
	return QString("%1, %2; %3, %4").arg(box.minCorner().x()).arg(box.minCorner().y()).arg(box.maxCorner().x()).arg(box.maxCorner().y());
}

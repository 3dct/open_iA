// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVec3.h"
// std
#include <ostream>

struct iAFeature
{
public:
					iAFeature();
	int				id;
	int				parentId;
	int				childId;
	double			volume;
	iAVec3d			centroid;
	iAVec3d			eigenvalues;
	iAVec3d			eigenvectors[3];
	iAVec3d			axesLength;
	double			bb[6];			// bounding box
	double			bbVolume;
	iAVec3d			bbSize;
	iAVec3d			obbVertices[8];	// oriented bounding box
	double			obbVolume;
	iAVec3d			obbSize;
};

std::ostream& operator<<(std::ostream& os, const iAFeature& obj);
bool operator>>(std::istream& is, iAFeature& obj);

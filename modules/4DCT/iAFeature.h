#pragma once
#include "iAVec3.h"
// std
#include <ostream>
#include <istream>

struct iAFeature
{
public:
					iAFeature();
	virtual			~iAFeature();
	int				id;
	int				parentId;
	int				childId;
	double			volume;
	Vec3d			centroid;
	Vec3d			eigenvalues;
	Vec3d			eigenvectors[3];
	Vec3d			axesLength;
	double			bb[6];			// bounding box
	double			bbVolume;
	Vec3d			bbSize;
	Vec3d			obbVertices[8];	// oriented bounding box
	double			obbVolume;
	Vec3d			obbSize;
};

std::ostream& operator<<(std::ostream& os, const iAFeature& obj);
bool operator>>(std::istream& is, iAFeature& obj);
#include "iAFeature.h"
// std
#include <sstream>

iAFeature::iAFeature()
	: volume{ 0 }
	, bbVolume{ 0 }
	, obbVolume{ 0 }
{
	id = parentId = childId = -1;
	bb[0] = 0; bb[1] = 0; bb[2] = 0; bb[3] = 0; bb[4] = 0; bb[5] = 0;
}

iAFeature::~iAFeature()
{ }

std::ostream& operator<<( std::ostream& os, const iAFeature& obj )
{
	os << obj.id << " ";
	os << obj.parentId << " ";
	os << obj.childId << " ";
	os << obj.volume << " ";
	os << obj.centroid[0] << " ";
	os << obj.centroid[1] << " ";
	os << obj.centroid[2] << " ";
	os << obj.eigenvalues[0] << " ";
	os << obj.eigenvalues[1] << " ";
	os << obj.eigenvalues[2] << " ";
	os << obj.eigenvectors[0][0] << " ";
	os << obj.eigenvectors[0][1] << " ";
	os << obj.eigenvectors[0][2] << " ";
	os << obj.eigenvectors[1][0] << " ";
	os << obj.eigenvectors[1][1] << " ";
	os << obj.eigenvectors[1][2] << " ";
	os << obj.eigenvectors[2][0] << " ";
	os << obj.eigenvectors[2][1] << " ";
	os << obj.eigenvectors[2][2] << " ";
	os << obj.axesLength[0] << " ";
	os << obj.axesLength[1] << " ";
	os << obj.axesLength[2] << " ";
	os << obj.bb[0] << " ";
	os << obj.bb[1] << " ";
	os << obj.bb[2] << " ";
	os << obj.bb[3] << " ";
	os << obj.bb[4] << " ";
	os << obj.bb[5] << " ";
	os << obj.bbVolume << " ";
	os << obj.bbSize[0] << " ";
	os << obj.bbSize[1] << " ";
	os << obj.bbSize[2] << " ";
	os << obj.obbVertices[0][0] << " ";
	os << obj.obbVertices[0][1] << " ";
	os << obj.obbVertices[0][2] << " ";
	os << obj.obbVertices[1][0] << " ";
	os << obj.obbVertices[1][1] << " ";
	os << obj.obbVertices[1][2] << " ";
	os << obj.obbVertices[2][0] << " ";
	os << obj.obbVertices[2][1] << " ";
	os << obj.obbVertices[2][2] << " ";
	os << obj.obbVertices[3][0] << " ";
	os << obj.obbVertices[3][1] << " ";
	os << obj.obbVertices[3][2] << " ";
	os << obj.obbVertices[4][0] << " ";
	os << obj.obbVertices[4][1] << " ";
	os << obj.obbVertices[4][2] << " ";
	os << obj.obbVertices[5][0] << " ";
	os << obj.obbVertices[5][1] << " ";
	os << obj.obbVertices[5][2] << " ";
	os << obj.obbVertices[6][0] << " ";
	os << obj.obbVertices[6][1] << " ";
	os << obj.obbVertices[6][2] << " ";
	os << obj.obbVertices[7][0] << " ";
	os << obj.obbVertices[7][1] << " ";
	os << obj.obbVertices[7][2] << " ";
	os << obj.obbVolume << " ";
	os << obj.obbSize[0] << " ";
	os << obj.obbSize[1] << " ";
	os << obj.obbSize[2] << "\n";
	return os;
}

bool operator>>(std::istream& is, iAFeature& obj)
{
	std::string line;
	if (!std::getline(is, line) || line == "")
	{
		return false;
	}
	std::istringstream iss(line);
	iss >> obj.id;
	iss >> obj.parentId;
	iss >> obj.childId;
	iss >> obj.volume;
	iss >> obj.centroid[0];
	iss >> obj.centroid[1];
	iss >> obj.centroid[2];
	iss >> obj.eigenvalues[0];
	iss >> obj.eigenvalues[1];
	iss >> obj.eigenvalues[2];
	iss >> obj.eigenvectors[0][0];
	iss >> obj.eigenvectors[0][1];
	iss >> obj.eigenvectors[0][2];
	iss >> obj.eigenvectors[1][0];
	iss >> obj.eigenvectors[1][1];
	iss >> obj.eigenvectors[1][2];
	iss >> obj.eigenvectors[2][0];
	iss >> obj.eigenvectors[2][1];
	iss >> obj.eigenvectors[2][2];
	iss >> obj.axesLength[0];
	iss >> obj.axesLength[1];
	iss >> obj.axesLength[2];
	iss >> obj.bb[0];
	iss >> obj.bb[1];
	iss >> obj.bb[2];
	iss >> obj.bb[3];
	iss >> obj.bb[4];
	iss >> obj.bb[5];
	iss >> obj.bbVolume;
	iss >> obj.bbSize[0];
	iss >> obj.bbSize[1];
	iss >> obj.bbSize[2];
	iss >> obj.obbVertices[0][0];
	iss >> obj.obbVertices[0][1];
	iss >> obj.obbVertices[0][2];
	iss >> obj.obbVertices[1][0];
	iss >> obj.obbVertices[1][1];
	iss >> obj.obbVertices[1][2];
	iss >> obj.obbVertices[2][0];
	iss >> obj.obbVertices[2][1];
	iss >> obj.obbVertices[2][2];
	iss >> obj.obbVertices[3][0];
	iss >> obj.obbVertices[3][1];
	iss >> obj.obbVertices[3][2];
	iss >> obj.obbVertices[4][0];
	iss >> obj.obbVertices[4][1];
	iss >> obj.obbVertices[4][2];
	iss >> obj.obbVertices[5][0];
	iss >> obj.obbVertices[5][1];
	iss >> obj.obbVertices[5][2];
	iss >> obj.obbVertices[6][0];
	iss >> obj.obbVertices[6][1];
	iss >> obj.obbVertices[6][2];
	iss >> obj.obbVertices[7][0];
	iss >> obj.obbVertices[7][1];
	iss >> obj.obbVertices[7][2];
	iss >> obj.obbVolume;
	iss >> obj.obbSize[0];
	iss >> obj.obbSize[1];
	iss >> obj.obbSize[2];
	return true;
}

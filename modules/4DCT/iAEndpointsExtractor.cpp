/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAEndpointsExtractor.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>
// std
#include <cassert>

typedef unsigned int ImageType;

void EndpointsExtractor::findTheMostFarPoint(vtkImageData * image, vtkImageData * stepsMap, const int pos[3], int * endpoint)
{
	ImageType * val = static_cast<ImageType *>(image->GetScalarPointer(pos[0], pos[1], pos[2]));

	int dims[3];
	image->GetDimensions(dims);

	for(int x = -1; x <= 1; x++) {
		for(int y = -1; y <= 1; y++) {
			for(int z = -1; z <= 1; z++) {
				int curPos[3];
				curPos[0] = pos[0] + x;
				curPos[1] = pos[1] + y;
				curPos[2] = pos[2] + z;

				if(curPos[0] < 0 || curPos[1] < 0 || curPos[2] < 0 ||
					curPos[0] >= dims[0] || curPos[1] >= dims[1] || curPos[2] >= dims[2])
				{
					continue;
				}

				char * mapVal = static_cast<char *>(stepsMap->GetScalarPointer(curPos));
				if(mapVal[0] != 0) continue;

				ImageType * voxel = static_cast<ImageType *>(image->GetScalarPointer(curPos));
				if(voxel[0] == val[0])
				{
					mapVal[0] = 1;
					findTheMostFarPoint(image, stepsMap, pos, endpoint);
					return;
				}
			}
		}
	}

	endpoint[0] = pos[0];
	endpoint[1] = pos[1];
	endpoint[2] = pos[2];
}

Endpoint EndpointsExtractor::procceedFiber(vtkImageData * image, vtkImageData * stepsMap, const int pos[3])
{
	int distantPnt[2][3];
	distantPnt[0][0] = pos[0];
	distantPnt[0][1] = pos[1];
	distantPnt[0][2] = pos[2];
	distantPnt[1][0] = pos[0];
	distantPnt[1][1] = pos[1];
	distantPnt[1][2] = pos[2];
	int curDistantPnt = 0;

	ImageType * val = static_cast<ImageType *>(image->GetScalarPointer(pos[0], pos[1], pos[2]));

	int dims[3];
	image->GetDimensions(dims);

	for(int x = -1; x <= 1; x++) {
		for(int y = -1; y <= 1; y++) {
			for(int z = -1; z <= 1; z++) {
				if(curDistantPnt > 2) {
					assert(false);		// FixMe
					continue;
				}

				int curPos[3];
				curPos[0] = pos[0] + x;
				curPos[1] = pos[1] + y;
				curPos[2] = pos[2] + z;

				if(curPos[0] < 0 || curPos[1] < 0 || curPos[2] < 0 ||
					curPos[0] >= dims[0] || curPos[1] >= dims[1] || curPos[2] >= dims[2])
				{
					continue;
				}

				char * mapVal = static_cast<char *>(stepsMap->GetScalarPointer(curPos));
				if(mapVal[0] != 0) continue;

				ImageType * voxel = static_cast<ImageType *>(image->GetScalarPointer(curPos));
				if(voxel[0] == val[0])
				{
					mapVal[0] = 1;
					findTheMostFarPoint(image, stepsMap, curPos, distantPnt[curDistantPnt++]);
				}
			}
		}
	}

	Endpoint endPnt;
	endPnt.SetPosition(distantPnt[0]);
	endPnt.SetPosition(distantPnt[1]);

	return endPnt;

	// get direction
}

inline bool EndpointsExtractor::isWithinExtent(const int coordinate[3], const int extent[6])
{
	for(int idx = 0; idx < 3; ++idx)
	{
		if( coordinate[idx] < extent[idx * 2] ||
			coordinate[idx] > extent[idx * 2 + 1])
		{
			return false;
		}
	}
	return true;
}

bool EndpointsExtractor::ranAway(vtkImageData * image, vtkImageData * stepMap, const int pos[3], int pathLength, int outPosition[3])
{
	ImageType * val = static_cast<ImageType *>(image->GetScalarPointer(pos[0], pos[1], pos[2]));
	int curPos[3];

	for(int x = -1; x <= 1; x++) {
		for(int y = -1; y <= 1; y++) {
			for(int z = -1; z <= 1; z++) {
				if(x == 0 && y == 0 && z == 0) continue;

				curPos[0] = pos[0] + x;
				curPos[1] = pos[1] + y;
				curPos[2] = pos[2] + z;

				int extent[6];
				image->GetExtent(extent);
				if(!isWithinExtent(curPos, extent)) continue;

				char * mapVal = static_cast<char *>(stepMap->GetScalarPointer(curPos));
				if(mapVal[0] != 0) continue;
				mapVal[0] = 1;

				ImageType * neighbourVal = static_cast<ImageType *>(image->GetScalarPointer(curPos));
				if(neighbourVal[0] == val[0]) {
					if(pathLength == 1)	{
						outPosition[0] = curPos[0];
						outPosition[1] = curPos[1];
						outPosition[2] = curPos[2];

						return true;
					} else {
						return ranAway(image, stepMap, curPos, pathLength - 1, outPosition);
					}
				}
			}
		}
	}

	return false;
}

bool EndpointsExtractor::getRemotePoint(vtkImageData * image, const int pos[3], int pathLength, int outPosition[3])
{
	assert(pathLength > 0);			// FixMe
	if(pathLength < 0) return false;

	int extent[6] = {
		pos[0] - pathLength, 
		pos[0] + pathLength, 
		pos[1] - pathLength, 
		pos[1] + pathLength, 
		pos[2] - pathLength, 
		pos[2] + pathLength
	};

	// allocate step map
	vtkSmartPointer<vtkImageData> stepMap = vtkSmartPointer<vtkImageData>::New();
	stepMap->SetExtent(extent);
	stepMap->AllocateScalars(VTK_CHAR, 1);
	void * buffer = stepMap->GetScalarPointer();
	int dim[3];
	stepMap->GetDimensions(dim);
	memset(buffer, 0, sizeof(char) * dim[0] * dim[1] * dim[2]);
	char * mapVal = static_cast<char *>(stepMap->GetScalarPointer(pos[0], pos[1], pos[2]));
	// mark as visited
	mapVal[0] = 1;

	return ranAway(image, stepMap, pos, pathLength, outPosition);
}

bool EndpointsExtractor::isAnEndpoint(vtkImageData * image, const int pos[3])
{
	ImageType * val = static_cast<ImageType *>(image->GetScalarPointer(pos[0], pos[1], pos[2]));
	int curPos[3];
	int neighbours = 0;

	int extent[6];
	image->GetExtent(extent);

	for(int x = -1; x <= 1; x++) {	
		for(int y = -1; y <= 1; y++) {			
			for(int z = -1; z <= 1; z++) {
				if(x ==  0 && y ==  0 && z ==  0) continue;

				curPos[0] = pos[0] + x;
				curPos[1] = pos[1] + y;
				curPos[2] = pos[2] + z;

				if(!isWithinExtent(curPos, extent)) continue;
				
				ImageType * neighbourVal = static_cast<ImageType *>(image->GetScalarPointer(curPos));
				if(neighbourVal[0] == val[0]) neighbours++;
			}
		}
	}

	return (neighbours == 1);
}

inline void CalculateDirection(const int startPnt[3], const int endPnt[3], double dir[3])
{
	dir[0] = endPnt[0] - startPnt[0];
	dir[1] = endPnt[1] - startPnt[1];
	dir[2] = endPnt[2] - startPnt[2];

	// normolize
	double length = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
	dir[0] /= length;
	dir[1] /= length;
	dir[2] /= length;
}

bool EndpointsExtractor::extract(vtkImageData * image, std::vector<Endpoint>& endpoints)
{
	if(image == nullptr)
	{
		assert(false);			// FixMe
		return false;
	}

	int dims[3];
	int extent[6];
	image->GetDimensions(dims);
	image->GetExtent(extent);

	//std::vector<ImageType> passedFibers;

	vtkSmartPointer<vtkImageData> stepsMap = vtkSmartPointer<vtkImageData>::New();
	stepsMap->SetDimensions(dims);
	stepsMap->AllocateScalars(VTK_CHAR, 1);
	void * buffer = stepsMap->GetScalarPointer();
	memset(buffer, 0, sizeof(char) * dims[0] * dims[1] * dims[2]);

	for(int x = extent[0]; x < extent[1]; x++) {
		for(int y = extent[2]; y < extent[3]; y++) {
			for(int z = extent[4]; z < extent[5]; z++) {
				int pos[3] = { x, y, z };

				char * mapVal = static_cast<char *>(stepsMap->GetScalarPointer(pos));
				//check is it visited
				if(mapVal[0] != 0) continue;
				//mark as visited
				mapVal[0] = 1;

				ImageType * voxel = static_cast<ImageType *>(image->GetScalarPointer(pos));
				//if voxel is the background
				if(voxel[0] == 0) continue;

				//it is a fiber
				//endpoints.push_back(ProcceedFiber(image, stepsMap, pos));
				if(isAnEndpoint(image, pos)) {
					// calculate direction
					int remotePoint[3];
					if(!getRemotePoint(image, pos, 5, remotePoint)) continue;
					double dir[3];
					CalculateDirection(remotePoint, pos, dir);

					Endpoint p;
					p.SetPosition(pos);
					p.SetDirection(dir);
					endpoints.push_back(p);
				}
			}
		}
	}

	return true;
}
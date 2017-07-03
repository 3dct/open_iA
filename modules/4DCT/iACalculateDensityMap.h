/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

// std
#include <vector>
// vtk
#include <vtkImageData.h>

template<class TPrecision, class TScalar>
class CalculateDensityMap
{
public:
	static std::vector<std::vector<std::vector<TPrecision>>>
		Calculate(vtkImageData* mask, int* gridSize, double* cellSize);
};

template<class TPrecision, class TScalar>
std::vector<std::vector<std::vector<TPrecision>>> CalculateDensityMap<TPrecision, TScalar>::Calculate(vtkImageData* mask, int* gridSize, double* cellSize)
{
	TScalar* buffer = (TScalar*)mask->GetScalarPointer();
	int extent[6];
	mask->GetExtent(extent);
	int size[3];
	size[0] = extent[1] - extent[0] + 1;
	size[1] = extent[3] - extent[2] + 1;
	size[2] = extent[5] - extent[4] + 1;

	std::vector<std::vector<std::vector<TPrecision>>> density;
	for (int i = 0; i < gridSize[0]; i++)
	{
		std::vector<std::vector<TPrecision>> subVec;
		for (int j = 0; j < gridSize[1]; j++)
		{
			std::vector<TPrecision> subSubVec;
			for (int k = 0; k < gridSize[2]; k++)
			{
				subSubVec.push_back(0);
			}
			subVec.push_back(subSubVec);
		}
		density.push_back(subVec);
	}

	//double cellSize[3];
	cellSize[0] = (double)size[0] / gridSize[0];
	cellSize[1] = (double)size[1] / gridSize[1];
	cellSize[2] = (double)size[2] / gridSize[2];

	for (int x = 0; x < size[0]; x++)
	{
		for (int y = 0; y < size[1]; y++)
		{
			for (int z = 0; z < size[2]; z++)
			{
				TScalar* val = (TScalar*)mask->GetScalarPointer(x, y, z);
				if (val[0] > 0)
				{
					int gridX = (double)x / cellSize[0];
					int gridY = (double)y / cellSize[1];
					int gridZ = (double)z / cellSize[2];

					density[gridX][gridY][gridZ] += 1;
				}
			}
		}
	}

	return density;

	/*TScalar* buffer = (TScalar*)mask->GetScalarPointer();
	int extent[6];
	mask->GetExtent(extent);
	int size[3];
	size[0] = extent[1] - extent[0];
	size[1] = extent[3] - extent[2];
	size[2] = extent[5] - extent[4];

	int densitySize[3];
	densitySize[0] = (int)(size[0] / gridSize) + 1;
	densitySize[1] = (int)(size[1] / gridSize) + 1;
	densitySize[2] = (int)(size[2] / gridSize) + 1;

	std::vector<std::vector<std::vector<TPrecision>>> density;
	for (int i = 0; i < densitySize[0]; i++)
	{
		std::vector<std::vector<TPrecision>> subVec;
		for (int j = 0; j < densitySize[1]; j++)
		{
			std::vector<TPrecision> subSubVec;
			for (int k = 0; k < densitySize[2]; k++)
			{
				subSubVec.push_back(0);
			}
			subVec.push_back(subSubVec);
		}
		density.push_back(subVec);
	}

	for (int x = 0; x < size[0]; x++)
	{
		for (int y = 0; y < size[1]; y++)
		{
			for (int z = 0; z < size[2]; z++)
			{
				TScalar* val = (TScalar*)mask->GetScalarPointer(x, y, z);
				if (val[0] > 0)
				{
					int gridX = x / gridSize;
					int gridY = y / gridSize;
					int gridZ = z / gridSize;

					density[gridX][gridY][gridZ] += 1;
				}
			}
		}
	}

	return density;*/
}

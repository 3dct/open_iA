// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	//TScalar* buffer = (TScalar*)mask->GetScalarPointer();
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

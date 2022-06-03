/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAVRMetrics.h"

#include <vtkSmartPointer.h>
#include <unordered_map>

//!This class calculates the metrics used in the Model in Miniature Heatmap
class iAVROctreeMetrics : public iAVRMetrics
{
public:
	iAVROctreeMetrics(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	std::vector<std::vector<std::vector<double>>>* getRegionAverage(vtkIdType octreeLevel, vtkIdType feature);
	std::vector<std::vector<std::vector<vtkIdType>>>* getMaxCoverageFiberPerRegion();
	std::vector<double> getMinMaxAvgRegionValues(vtkIdType octreeLevel, vtkIdType feature);
	std::vector<double> getRegionValues(vtkIdType octreeLevel, vtkIdType region, vtkIdType feature);
	std::vector<std::vector<std::vector<double>>>* getJaccardIndex(vtkIdType octreeLevel);
	double getMaxNumberOfFibersInRegion(vtkIdType octreeLevel);

private:
	//Stores for the [octree level] in an [octree region] the fibers which have the max coverage (Every Fiber can only be in one region)
	std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage;
	//Stores for an [octree level] for choosen [feature] for every [octree region] the metric value
	std::vector<std::vector<std::vector<double>>>* m_calculatedAverage;
	//Stores for the [octree level] in an [octree region] its Jaccard index to another [octree region]
	std::vector<std::vector<std::vector<double>>>* m_jaccardValues;
	//Stores for the [octree level] the max amount of fibers which lie in an octree region
	std::vector<double>* m_maxNumberOffibersInRegions;
	//Stores the info if at a specific octree [level] a specific [feature] is already calculated
	std::vector<std::vector<bool>>* isAlreadyCalculated;

	void calculateWeightedAverage(vtkIdType octreeLevel, vtkIdType feature);
	void calculateMaxCoverageFiberPerRegion();
	void findBiggestCoverage(vtkIdType level, vtkIdType fiber);
	void calculateJaccardIndex(vtkIdType level);
	double calculateJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	double calculateWeightedJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	double calculateJaccardDistance(vtkIdType level, vtkIdType region1, vtkIdType region2);
	void calculateMaxNumberOfFibersInRegion();
};
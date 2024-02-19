// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRMetrics.h"

//! This class calculates the metrics used in the Model in Miniature Heatmap
class iAVROctreeMetrics : public iAVRMetrics
{
public:
	iAVROctreeMetrics(vtkTable* objectTable, std::vector<iAVROctree*> const & octrees);
	std::vector<std::vector<std::vector<double>>> const & getRegionAverage(vtkIdType octreeLevel, vtkIdType feature);
	std::vector<std::vector<std::vector<vtkIdType>>> const & getMaxCoverageFiberPerRegion();
	std::vector<double> getMinMaxAvgRegionValues(vtkIdType octreeLevel, vtkIdType feature);
	std::vector<double> getRegionValues(vtkIdType octreeLevel, vtkIdType region, vtkIdType feature);
	std::vector<std::vector<std::vector<double>>> const & getJaccardIndex(vtkIdType octreeLevel);
	double getMaxNumberOfFibersInRegion(vtkIdType octreeLevel);

private:
	//! Stores for the [octree level] in an [octree region] the fibers which have the max coverage (Every Fiber can only be in one region)
	std::vector<std::vector<std::vector<vtkIdType>>> m_maxCoverage;
	//! Stores for an [octree level] for choosen [feature] for every [octree region] the metric value
	std::vector<std::vector<std::vector<double>>> m_calculatedAverage;
	//! Stores for the [octree level] in an [octree region] its Jaccard index to another [octree region]
	std::vector<std::vector<std::vector<double>>> m_jaccardValues;
	//! Stores for the [octree level] the max amount of fibers which lie in an octree region
	std::vector<double> m_maxNumberOfFibersInRegions;
	//! Stores the info if at a specific octree [level] a specific [feature] is already calculated
	std::vector<std::vector<bool>> m_isAlreadyCalculated;

	void calculateWeightedAverage(vtkIdType octreeLevel, vtkIdType feature);
	void calculateMaxCoverageFiberPerRegion();
	void findBiggestCoverage(vtkIdType level, vtkIdType fiber);
	void calculateJaccardIndex(vtkIdType level);
	double calculateJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	double calculateWeightedJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	double calculateJaccardDistance(vtkIdType level, vtkIdType region1, vtkIdType region2);
	void calculateMaxNumberOfFibersInRegion();
};

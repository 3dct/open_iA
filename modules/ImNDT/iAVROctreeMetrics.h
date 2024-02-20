// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRMetrics.h"

//! This class calculates the metrics used in the Model in Miniature Heatmap
class iAVROctreeMetrics : public iAVRMetrics
{
public:
	iAVROctreeMetrics(vtkTable* objectTable, std::vector<iAVROctree*> const & octrees);
	//! Returns the average of each region (in the octree) for a feature as vector
	//! @return vector which stores for an [octree level] for choosen [feature] for every [octree region] the metric value
	std::vector<std::vector<std::vector<double>>> const & getRegionAverage(vtkIdType octreeLevel, vtkIdType feature);
	//! Returns the fiber with the maximum coverage of each region (in every octree level)
	std::vector<std::vector<std::vector<vtkIdType>>> const & getMaxCoverageFiberPerRegion();
	//! Returns the min and max average for all regions (in one octree level and feature)
	//! @return vector which stores min [0] and max [1] value of the given feature.
	std::vector<double> getMinMaxAvgRegionValues(vtkIdType octreeLevel, vtkIdType feature);
	//! Returns the values of a feature for all fibers in the given region
	//! Vector stores all instances (values) of the choosen feature
	std::vector<double> getRegionValues(vtkIdType octreeLevel, vtkIdType region, vtkIdType feature);
	//! Returns the jaccard index of an octree level. Stores previously computed level.
	std::vector<std::vector<std::vector<double>>> const & getJaccardIndex(vtkIdType octreeLevel);
	//! Returns amount of fibers for the region with the most fibers (of a given octree level)
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

	//! Calculates the weighted average of every octree region for a given feature at a given octree level.
	//! In the first call the metric has to calculate the values, for later calls the metric is saved
	//! Calculates:  1/N * SUM[N]( Attribut * weight)  with N = all Fibers in the region
	void calculateWeightedAverage(vtkIdType octreeLevel, vtkIdType feature);
	//! Returns a vector which stores for each fiber its region with the strongest coverage fo every level
	//! So every fiber is only stored once for a level!
	void calculateMaxCoverageFiberPerRegion();
	//! Returns the biggest coverage value for a specific fiber over all regions at the given octree level
	void findBiggestCoverage(vtkIdType level, vtkIdType fiber);
	//! Iterates through all permutations of region pairs and calculates the jaccard index (calls calculateJaccardIndex with two distinct regions)
	void calculateJaccardIndex(vtkIdType level);
	//! Calculates the size of the intersection divided by the size of the union of the chosen regions;
	//! calculated from the fiber intersection data.
	//! @return value between 0 and 1; 1 if the regions are the same
	double calculateJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	//! Calculates the size of the intersection divided by the size of the union of the chosen regions
	//! Uses the coverage of each fiber from the intersection data as weights.
	//! @return value between 0 and 1; 1 if the regions are the same
	double calculateWeightedJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2);
	//! Measures the dissimilarity (1 - jaccard index)
	double calculateJaccardDistance(vtkIdType level, vtkIdType region1, vtkIdType region2);
	//! Iterates through all regions in all level and counts the amount of fibers. The maximum amount of fibers in a region (in a level) is stored.
	void calculateMaxNumberOfFibersInRegion();
};

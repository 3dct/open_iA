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
#include "iAVROctreeMetrics.h"

#include <iALog.h>

iAVROctreeMetrics::iAVROctreeMetrics(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees) : iAVRMetrics(objectTable, io,
	octrees)
{
	//Initialize vectors
	isAlreadyCalculated = new std::vector<std::vector<bool>>(m_octrees->size(), std::vector<bool>(numberOfFeatures, false));

	std::vector<double> region = std::vector<double>();
	std::vector<std::vector<double>> feature = std::vector<std::vector<double>>(numberOfFeatures, region);
	m_calculatedAverage = new std::vector<std::vector<std::vector<double>>>(m_octrees->size(), feature);
	m_maxCoverage = new std::vector<std::vector<std::vector<vtkIdType>>>();
	m_jaccardValues = new std::vector<std::vector<std::vector<double>>>(m_octrees->size());
	m_maxNumberOffibersInRegions = new std::vector<double>();
}

//! Calculates the weighted average of every octree region for a given feature at a given octree level.
//! In the first call the metric has to calculate the values, for later calls the metric is saved
//! Calculates:  1/N * SUM[N]( Attribut * weight)  with N = all Fibers in the region
void iAVROctreeMetrics::calculateWeightedAverage(vtkIdType octreeLevel, vtkIdType feature)
{
	if (!isAlreadyCalculated->at(octreeLevel).at(feature)) {
		//LOG(lvlDebug,QString("Possible Regions: %1\n").arg(m_fiberCoverage->at(octreeLevel).size()));

		for (size_t region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		{
			double metricResultPerRegion = 0;
			int fibersInRegion = 0;
			//LOG(lvlDebug,QString(">>> REGION %1 <<<\n").arg(region));

			for (auto element : *m_fiberCoverage->at(octreeLevel).at(region))
			{
				//LOG(lvlDebug,QString("[%1] --- %2 \%").arg(element.first).arg(element.second));

				//double fiberAttribute = m_objectTable->GetValue(element.first, m_io.getOutputMapping()->value(feature)).ToFloat();
				double fiberAttribute = m_objectTable->GetValue(element.first, feature).ToFloat();
				double weightedAttribute = fiberAttribute * element.second;

				metricResultPerRegion += weightedAttribute;
				fibersInRegion++;
			}
			if (fibersInRegion == 0)
			{
				fibersInRegion = 1; // Prevent Division by zero
			}

			//LOG(lvlDebug,QString("Region [%1] --- %2 \%\n").arg(region).arg(metricResultPerRegion / fibersInRegion));

			m_calculatedAverage->at(octreeLevel).at(feature).push_back(metricResultPerRegion / fibersInRegion);
		}
		isAlreadyCalculated->at(octreeLevel).at(feature) = true;
	}
}

//! Returns the average of each region (in the octree) for a feature as vector
//! Vector stores for an [octree level] for choosen [feature] for every [octree region] the metric value
std::vector<std::vector<std::vector<double>>>* iAVROctreeMetrics::getRegionAverage(vtkIdType octreeLevel, vtkIdType feature)
{
	calculateWeightedAverage(octreeLevel, feature);

	return m_calculatedAverage;
}

//! Returns the fiber with the maximum coverage of each region (in every octree level)
std::vector<std::vector<std::vector<vtkIdType>>>* iAVROctreeMetrics::getMaxCoverageFiberPerRegion()
{
	if (m_maxCoverage->empty()) {
		calculateMaxCoverageFiberPerRegion();
		return m_maxCoverage;
	}
	else
	{
		return m_maxCoverage;
	}
}

//! Returns the min and max average for all regions (in one octree level and feature)
//! Vector stores min [0] and max [1] value of the given feature.
std::vector<double> iAVROctreeMetrics::getMinMaxAvgRegionValues(vtkIdType octreeLevel, vtkIdType feature)
{
	std::vector<double> minMax = std::vector<double>(2);
	auto minMaxElem = std::minmax_element(m_calculatedAverage->at(octreeLevel).at(feature).begin(), m_calculatedAverage->at(octreeLevel).at(feature).end());
	minMax.at(0) = *minMaxElem.first;
	minMax.at(1) = *minMaxElem.second;

	return minMax;
}

//! Returns the values of a feature for all fibers in the given region
//! Vector stores all instances (values) of the choosen feature
std::vector<double> iAVROctreeMetrics::getRegionValues(vtkIdType octreeLevel, vtkIdType region, vtkIdType feature)
{
	auto fibersInRegion = m_fiberCoverage->at(octreeLevel).at(region);
	std::vector<double> values = std::vector<double>();

	for (auto fiber : *fibersInRegion)
	{
		auto value = m_objectTable->GetValue(fiber.first, feature).ToFloat();
		values.push_back(value);
	}

	return values;
}

//! Returns the jaccard index of an octree level. Stores previously computed level.
std::vector<std::vector<std::vector<double>>>* iAVROctreeMetrics::getJaccardIndex(vtkIdType octreeLevel)
{
	if (m_jaccardValues->at(octreeLevel).empty())
	{
		calculateJaccardIndex(octreeLevel);
		return m_jaccardValues;
	}
	else
	{
		return m_jaccardValues;
	}
}

//! Returns amount of fibers for the region with the most fibers (of a given octree level)
double iAVROctreeMetrics::getMaxNumberOfFibersInRegion(vtkIdType octreeLevel)
{
	if (m_maxNumberOffibersInRegions->empty())
	{
		calculateMaxNumberOfFibersInRegion();
	}

	return m_maxNumberOffibersInRegions->at(octreeLevel);
}

//! Returns a vector which stores for each fiber its region with the strongest coverage fo every level
//! So every fiber is only stored once for a level!
void iAVROctreeMetrics::calculateMaxCoverageFiberPerRegion()
{
	//Initialize new Vectors
	for (size_t level = 0; level < m_fiberCoverage->size(); level++)
	{
		//Initialize the region vec for every level
		m_maxCoverage->push_back(std::vector<std::vector<vtkIdType>>());

		for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			//Initialize a vec of IDs for every region
			m_maxCoverage->at(level).push_back(std::vector<vtkIdType>());
		}
	}

	for (size_t level = 0; level < m_fiberCoverage->size(); level++)
	{
		for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
		{
			findBiggestCoverage(level, row);
		}
	}
}

//! Returns the biggest coverage value for a specific fiber over all regions at the given octree level
void iAVROctreeMetrics::findBiggestCoverage(vtkIdType level, vtkIdType fiber)
{
	double currentMaxCoverage = -1.0;
	vtkIdType regionWithMaxCoverage = 0;

	for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		auto it = m_fiberCoverage->at(level).at(region)->find(fiber);
		//If fiber has a coverage...
		if (it != m_fiberCoverage->at(level).at(region)->end())
		{
			if (currentMaxCoverage < it->second)
			{
				currentMaxCoverage = it->second;
				regionWithMaxCoverage = region;
			}

			//fibers is fully contained in one region - stop searching
			if (currentMaxCoverage >= 1.0) break;
		}
	}

	m_maxCoverage->at(level).at(regionWithMaxCoverage).push_back(fiber);
}

//! Iterates through all permutations of region pairs and calculates the jaccard index (calls calculateJaccardIndex with two distinct regions)
void iAVROctreeMetrics::calculateJaccardIndex(vtkIdType level)
{
	for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		m_jaccardValues->at(level).push_back(std::vector<double>());

		// If only one Region
		if (m_fiberCoverage->at(level).size() == 1)
		{
			m_jaccardValues->at(level).at(region).push_back(1);
			return;
		}

		for (size_t region2 = 0; region2 < m_fiberCoverage->at(level).size(); region2++)
		{
			double index = 0;

			index = calculateJaccardIndex(level, region, region2);

			m_jaccardValues->at(level).at(region).push_back(index);

			//LOG(lvlDebug,QString("jaccardValue for [%1][%2] is %3").arg(region).arg(region2).arg(test));
			//LOG(lvlDebug,QString("Weighted jaccardValue for [%1][%2] is %3\n").arg(region).arg(region2).arg(m_jaccardValues->at(level).at(region).at(region2)));
		}
	}
}

//! Calculates the size of the intersection divided by the size of the union of the chosen regions
//! Gets calculated from the fiber intersection data. Value lies between 0 and 1.
//! Returns 1 if the region are the same
double iAVROctreeMetrics::calculateJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	if (region1 == region2) return 1.0;

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	double sizeRegion1 = fibersInRegion1->size();
	double sizeRegion2 = fibersInRegion2->size();

	if (sizeRegion1 == 0 || sizeRegion2 == 0) return 0.0;

	double sizeintersection = 0;

	for (auto fiber : *fibersInRegion1)
	{
		if (fibersInRegion2->count(fiber.first) == 1)
		{
			sizeintersection++;
		}
	}

	double jaccard_index = sizeintersection / (sizeRegion1 + sizeRegion2 - sizeintersection);

	return jaccard_index;
}

//! Calculates the size of the intersection divided by the size of the union of the chosen regions
//! Uses the coverage of each fiber from the intersection data as weights. Value lies between 0 and 1.
//! Returns 1 if the region are the same
double iAVROctreeMetrics::calculateWeightedJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	if (region1 == region2) return 1.0;

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	double sizeRegion1 = fibersInRegion1->size();
	double sizeRegion2 = fibersInRegion2->size();
	double sizeShared = 0;

	if (sizeRegion1 == 0 || sizeRegion2 == 0) return 0.0;

	double fibersOfRegion1 = 0;
	double fibersOfRegion2 = 0;
	double sharedfibers = 0;

	//Count in region 1 the shared fibers (to region2), save them combined and individual and then delete them...
	for (auto fiber : *fibersInRegion1)
	{
		fibersOfRegion1 += fiber.second;

		if (fibersInRegion2->count(fiber.first) == 1)
		{
			sizeShared++;
			sharedfibers += fiber.second;
			sharedfibers += fibersInRegion2->at(fiber.first);
		}
	}

	if (sharedfibers == 0) return 0.0; // nothing in common

	//.. then sum up the remaining
	for (auto fiber : *fibersInRegion2)
	{
		fibersOfRegion2 += fiber.second;
	}

	//Divide to stay between 0 and 1
	//sharedfibers = sharedfibers / sizeShared;
	//fibersOfRegion1 = fibersOfRegion1 / sizeRegion1;
	//fibersOfRegion2 = fibersOfRegion2 / sizeRegion2;

	double weightedJaccard_index = sharedfibers / ((fibersOfRegion1 + fibersOfRegion2)) / (sizeRegion1 + sizeRegion2);
	//double weightedJaccard_index = sharedfibers / (fibersOfRegion1 + fibersOfRegion2 - sharedfibers);

	return weightedJaccard_index;
}

//! Measures the dissimilarity (1 - jaccard index)
double iAVROctreeMetrics::calculateJaccardDistance(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	return 1 - calculateJaccardIndex(level, region1, region2);
}

//! Iterates through all regions in all level and counts the amount of fibers. The maximum amount of fibers in a region (in a level) is stored.
void iAVROctreeMetrics::calculateMaxNumberOfFibersInRegion()
{
	for (size_t level = 0; level < m_octrees->size(); level++)
	{
		double numberOfFibers = 0;

		for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			auto fibers = m_fiberCoverage->at(level).at(region)->size();

			if (numberOfFibers < fibers) numberOfFibers = fibers;
		}
		m_maxNumberOffibersInRegions->push_back(numberOfFibers);
	}
}
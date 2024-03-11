// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVROctreeMetrics.h"

#include <vtkTable.h>
#include <vtkVariant.h>

iAVROctreeMetrics::iAVROctreeMetrics(vtkTable* objectTable,	std::vector<iAVROctree*> const & octrees) : iAVRMetrics(objectTable, octrees),
	m_maxCoverage(),
	m_calculatedAverage(m_octrees.size(), std::vector<std::vector<double>>(numberOfFeatures, std::vector<double>())),
	m_jaccardValues(m_octrees.size()),
	m_maxNumberOfFibersInRegions(),
	m_isAlreadyCalculated(m_octrees.size(), std::vector<bool>(numberOfFeatures, false))
{
}

void iAVROctreeMetrics::calculateWeightedAverage(vtkIdType octreeLevel, vtkIdType feature)
{
	if (!m_isAlreadyCalculated.at(octreeLevel).at(feature)) {
		//LOG(lvlDebug,QString("Possible Regions: %1\n").arg(m_objectCoverage->at(octreeLevel).size()));

		for (size_t region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		{
			double metricResultPerRegion = 0;
			int fibersInRegion = 0;
			//LOG(lvlDebug,QString(">>> REGION %1 <<<\n").arg(region));

			for (auto const & element : *m_fiberCoverage->at(octreeLevel).at(region))
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

			m_calculatedAverage.at(octreeLevel).at(feature).push_back(metricResultPerRegion / fibersInRegion);
		}
		m_isAlreadyCalculated.at(octreeLevel).at(feature) = true;
	}
}

std::vector<std::vector<std::vector<double>>> const & iAVROctreeMetrics::getRegionAverage(vtkIdType octreeLevel, vtkIdType feature)
{
	calculateWeightedAverage(octreeLevel, feature);

	return m_calculatedAverage;
}

std::vector<std::vector<std::vector<vtkIdType>>> const & iAVROctreeMetrics::getMaxCoverageFiberPerRegion()
{
	if (m_maxCoverage.empty())
	{
		calculateMaxCoverageFiberPerRegion();
		return m_maxCoverage;
	}
	else
	{
		return m_maxCoverage;
	}
}

std::vector<double> iAVROctreeMetrics::getMinMaxAvgRegionValues(vtkIdType octreeLevel, vtkIdType feature)
{
	std::vector<double> minMax = std::vector<double>(2);
	//If Level is not calculated do computation
	getRegionAverage(octreeLevel, feature);
	auto minMaxElem = std::minmax_element(m_calculatedAverage.at(octreeLevel).at(feature).begin(), m_calculatedAverage.at(octreeLevel).at(feature).end());
	minMax.at(0) = *minMaxElem.first;
	minMax.at(1) = *minMaxElem.second;

	return minMax;
}

std::vector<double> iAVROctreeMetrics::getRegionValues(vtkIdType octreeLevel, vtkIdType region, vtkIdType feature)
{
	auto fibersInRegion = m_fiberCoverage->at(octreeLevel).at(region);
	std::vector<double> values = std::vector<double>();

	for (auto const & fiber : *fibersInRegion)
	{
		auto value = m_objectTable->GetValue(fiber.first, feature).ToFloat();
		values.push_back(value);
	}

	return values;
}

std::vector<std::vector<std::vector<double>>> const & iAVROctreeMetrics::getJaccardIndex(vtkIdType octreeLevel)
{
	if (m_jaccardValues.at(octreeLevel).empty())
	{
		calculateJaccardIndex(octreeLevel);
		return m_jaccardValues;
	}
	else
	{
		return m_jaccardValues;
	}
}

double iAVROctreeMetrics::getMaxNumberOfFibersInRegion(vtkIdType octreeLevel)
{
	if (m_maxNumberOfFibersInRegions.empty())
	{
		calculateMaxNumberOfFibersInRegion();
	}

	return m_maxNumberOfFibersInRegions.at(octreeLevel);
}

void iAVROctreeMetrics::calculateMaxCoverageFiberPerRegion()
{
	//Initialize new Vectors
	for (size_t level = 0; level < m_fiberCoverage->size(); level++)
	{
		//Initialize the region vec for every level
		m_maxCoverage.push_back(std::vector<std::vector<vtkIdType>>());

		for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			//Initialize a vec of IDs for every region
			m_maxCoverage.at(level).push_back(std::vector<vtkIdType>());
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

	m_maxCoverage.at(level).at(regionWithMaxCoverage).push_back(fiber);
}

void iAVROctreeMetrics::calculateJaccardIndex(vtkIdType level)
{
	for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		m_jaccardValues.at(level).push_back(std::vector<double>());

		// If only one Region
		if (m_fiberCoverage->at(level).size() == 1)
		{
			m_jaccardValues.at(level).at(region).push_back(1);
			return;
		}

		for (size_t region2 = 0; region2 < m_fiberCoverage->at(level).size(); region2++)
		{
			double index = 0;

			index = calculateJaccardIndex(level, region, region2);

			m_jaccardValues.at(level).at(region).push_back(index);

			//LOG(lvlDebug,QString("jaccardValue for [%1][%2] is %3").arg(region).arg(region2).arg(test));
			//LOG(lvlDebug,QString("Weighted jaccardValue for [%1][%2] is %3\n").arg(region).arg(region2).arg(m_jaccardValues->at(level).at(region).at(region2)));
		}
	}
}

double iAVROctreeMetrics::calculateJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	if (region1 == region2) return 1.0;

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	double sizeRegion1 = fibersInRegion1->size();
	double sizeRegion2 = fibersInRegion2->size();

	if (sizeRegion1 == 0 || sizeRegion2 == 0) return 0.0;

	double sizeintersection = 0;

	for (auto const & fiber : *fibersInRegion1)
	{
		if (fibersInRegion2->count(fiber.first) == 1)
		{
			sizeintersection++;
		}
	}

	double jaccard_index = sizeintersection / (sizeRegion1 + sizeRegion2 - sizeintersection);

	return jaccard_index;
}

double iAVROctreeMetrics::calculateWeightedJaccardIndex(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	if (region1 == region2) return 1.0;

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	double sizeRegion1 = fibersInRegion1->size();
	double sizeRegion2 = fibersInRegion2->size();
	//double sizeShared = 0;

	if (sizeRegion1 == 0 || sizeRegion2 == 0) return 0.0;

	double fibersOfRegion1 = 0;
	double fibersOfRegion2 = 0;
	double sharedfibers = 0;

	//Count in region 1 the shared fibers (to region2), save them combined and individual and then delete them...
	for (auto const & fiber : *fibersInRegion1)
	{
		fibersOfRegion1 += fiber.second;

		if (fibersInRegion2->count(fiber.first) == 1)
		{
			//sizeShared++;
			sharedfibers += fiber.second;
			sharedfibers += fibersInRegion2->at(fiber.first);
		}
	}

	if (sharedfibers == 0) return 0.0; // nothing in common

	//.. then sum up the remaining
	for (auto const & fiber : *fibersInRegion2)
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

double iAVROctreeMetrics::calculateJaccardDistance(vtkIdType level, vtkIdType region1, vtkIdType region2)
{
	return 1 - calculateJaccardIndex(level, region1, region2);
}

void iAVROctreeMetrics::calculateMaxNumberOfFibersInRegion()
{
	for (size_t level = 0; level < m_octrees.size(); level++)
	{
		double numberOfFibers = 0;

		for (size_t region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			auto fibers = m_fiberCoverage->at(level).at(region)->size();

			if (numberOfFibers < fibers) numberOfFibers = fibers;
		}
		m_maxNumberOfFibersInRegions.push_back(numberOfFibers);
	}
}

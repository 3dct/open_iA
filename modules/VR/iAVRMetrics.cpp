/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVRMetrics.h"

#include <iALog.h>

#include <vtkVariant.h>
#include <vtkProperty2D.h>
#include <vtkTextProperty.h>
#include <vtkAssembly.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPlaneSource.h>
#include <vtkCellData.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkGlyph3D.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkAppendPolyData.h>
#include <vtkDataSetMapper.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

iAVRMetrics::iAVRMetrics(vtkRenderer* renderer, vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees):m_renderer(renderer), m_objectTable(objectTable), m_io(io),
m_octrees(octrees)
{
	//numberOfFeatures = iACsvConfig::MappedCount;
	numberOfFeatures = m_objectTable->GetNumberOfColumns();

	//Initialize vectors
	isAlreadyCalculated = new std::vector<std::vector<bool>>(m_octrees->size(), std::vector<bool>(numberOfFeatures, false));
	
	std::vector<double> region = std::vector<double>();
	std::vector<std::vector<double>> feature = std::vector<std::vector<double>>(numberOfFeatures, region);
	m_calculatedAverage = new std::vector<std::vector<std::vector<double>>>(m_octrees->size(), feature);
	m_maxCoverage = new std::vector<std::vector<std::vector<vtkIdType>>>();
	m_jaccardValues = new std::vector<std::vector<std::vector<double>>>(m_octrees->size());
	m_maxNumberOffibersInRegions = new std::vector<double>();
	m_currentHistogram = new std::vector<std::vector<int>>();
	m_histogramParameter = new HistogramParameters();

	mipPlanes = std::vector<vtkPolyData*>();

	storeMinMaxValues();

	titleTextSource = vtkSmartPointer<vtkTextActor3D>::New();
	textSource = vtkSmartPointer<vtkTextActor3D>::New();
	m_ColorBar = vtkSmartPointer<vtkActor>::New();

	m_colorBarVisible = false;
}

//! Has to be called *before* getting any Metric data
void iAVRMetrics::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

//! Calculates the weighted average of every octree region for a given feature at a given octree level.
//! In the first call the metric has to calculate the values, for later calls the metric is saved
//! Calculates:  1/N * SUM[N]( Attribut * weight)  with N = all Fibers in the region
void iAVRMetrics::calculateWeightedAverage(int octreeLevel, int feature)
{
	if(!isAlreadyCalculated->at(octreeLevel).at(feature)){

		//LOG(lvlDebug,QString("Possible Regions: %1\n").arg(m_fiberCoverage->at(octreeLevel).size()));

		for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
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
			if(fibersInRegion == 0)
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
std::vector<std::vector<std::vector<double>>>* iAVRMetrics::getRegionAverage(int octreeLevel, int feature)
{
	calculateWeightedAverage(octreeLevel, feature);

	return m_calculatedAverage;
}

vtkSmartPointer<vtkLookupTable> iAVRMetrics::getLut()
{
	return m_lut;
}

std::vector<std::vector<std::vector<vtkIdType>>>* iAVRMetrics::getMaxCoverageFiberPerRegion()
{
	if(m_maxCoverage->empty()){
		calculateMaxCoverageFiberPerRegion();
		return m_maxCoverage;
	}
	else
	{
		return m_maxCoverage;
	}
}

//! Returns the min [0] and the max [1] value of a feature as vector
std::vector<double> iAVRMetrics::getMinMaxFiberValues(int feature)
{
	std::vector<double> minMax = std::vector<double>(2);
	minMax.at(0) = m_minMaxValues->at(feature).at(0);
	minMax.at(1) = m_minMaxValues->at(feature).at(1);

	return minMax;
}

std::vector<double> iAVRMetrics::getMinMaxAvgRegionValues(int octreeLevel, int feature)
{
	std::vector<double> minMax = std::vector<double>(2);
	auto minMaxElem = std::minmax_element(m_calculatedAverage->at(octreeLevel).at(feature).begin(), m_calculatedAverage->at(octreeLevel).at(feature).end());
	minMax.at(0) = *minMaxElem.first;
	minMax.at(1) = *minMaxElem.second;

	return minMax;
}

int iAVRMetrics::getNumberOfFeatures()
{
	return numberOfFeatures;
}

QString iAVRMetrics::getFeatureName(int feature)
{
	//QString featureName = m_objectTable->GetColumnName(m_io.getOutputMapping()->value(feature));
	QString featureName = m_objectTable->GetColumnName(feature);
	return featureName;
}

std::vector<std::vector<std::vector<double>>>* iAVRMetrics::getWeightedJaccardIndex(int level)
{
	if(m_jaccardValues->at(level).empty())
	{
		calculateJaccardIndex(level, true);
		return m_jaccardValues;
	}
	else
	{
		return m_jaccardValues;
	}
	
}

double iAVRMetrics::getMaxNumberOfFibersInRegion(int level)
{
	if (m_maxNumberOffibersInRegions->empty())
	{
		calculateMaxNumberOfFibersInRegion();
	}

	return m_maxNumberOffibersInRegions->at(level);
}

//! Calculates the number of needed bins through Sturge's Rule
int iAVRMetrics::getMaxNumberOfHistogramBins(int level)
{
	double bins = 1 + 3.322 * log(getMaxNumberOfFibersInRegion(level));
	return round(bins);
}

//! Returns all parameters of the histogram calculation of the two given regions
//! If no feature are choosen (nullptr or empty) all avaiable features are calculated
HistogramParameters* iAVRMetrics::getHistogram(int level, std::vector<int>* featureList, int region1, int region2)
{
	//If no feature is choosen, calculate all
	if(featureList == nullptr || featureList->empty())
	{
		featureList = new std::vector<int>();

		for (int f = 0; f < numberOfFeatures; f++)
		{
			featureList->push_back(f);
		}
	}
	m_histogramParameter->featureList = featureList;

	calculateHistogramValues(level, featureList, region1, region2);

	return m_histogramParameter;
}


double iAVRMetrics::histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax)
{
	double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
	return result;
}

double iAVRMetrics::histogramNormalizationExpo(double value, double newMin, double newMax, double oldMin, double oldMax)
{
	newMin = log(newMin);
	newMax = log(newMax);

	double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
	return exp(result);
}

void iAVRMetrics::storeMinMaxValues()
{
	m_minMaxValues = new std::vector<std::vector<double>>();

	//int numberOfFeatures = m_objectTable->GetNumberOfColumns();

	std::vector<double> minAttribute = std::vector<double>(); //= m_objectTable->GetColumn(feature)->GetVariantValue(0).ToFloat();
	std::vector<double> maxAttribute = std::vector<double>(); //= minAttribute;

	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		for (int feature = 0; feature < numberOfFeatures; feature++)
		{
			//double currentValue = m_objectTable->GetColumn(feature)->GetVariantValue(row).ToFloat();
			//double currentValue = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(feature)).ToFloat();
			double currentValue = m_objectTable->GetValue(row, feature).ToFloat();

			//initialize values at first fiber
			if (row == 0)
			{
				minAttribute.push_back(currentValue);
				maxAttribute.push_back(currentValue);
			}
			if (minAttribute[feature] > currentValue)
			{
				minAttribute[feature] = currentValue;
			}
			if (maxAttribute[feature] < currentValue)
			{
				maxAttribute[feature] = currentValue;
			}
		}
	}
	for (int feature = 0; feature < numberOfFeatures; feature++)
	{
		std::vector<double> tempVec = std::vector<double>();
		tempVec.push_back(minAttribute[feature]);
		tempVec.push_back(maxAttribute[feature]);
		m_minMaxValues->push_back(tempVec);
	}
}

//! Returns a vector which stores for each fiber its region with the strongest coverage fo every level
//! So every fiber is only stored once for a level!
void iAVRMetrics::calculateMaxCoverageFiberPerRegion()
{
	//Initialize new Vectors
	for (int level = 0; level < m_fiberCoverage->size(); level++)
	{
		//Initialize the region vec for every level
		m_maxCoverage->push_back(std::vector<std::vector<vtkIdType>>());

		for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			//Initialize a vec of IDs for every region
			m_maxCoverage->at(level).push_back(std::vector<vtkIdType>());
		}

	}
	
	for (int level = 0; level < m_fiberCoverage->size(); level++)
	{
		for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
		{
			findBiggestCoverage(level, row);
		}

	}
}

//! Returns the biggest coverage value for a specific fiber over all regions at the given octree level
void iAVRMetrics::findBiggestCoverage(int level, int fiber)
{
	double currentMaxCoverage = -1.0;
	int regionWithMaxCoverage = 0;

	for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
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

void iAVRMetrics::calculateJaccardIndex(int level, bool weighted)
{
	for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
	{
		m_jaccardValues->at(level).push_back(std::vector<double>());

		// If only one Region
		if(m_fiberCoverage->at(level).size() == 1)
		{
			m_jaccardValues->at(level).at(region).push_back(1);
			return;
		}

		for (int region2 = 0; region2 < m_fiberCoverage->at(level).size(); region2++)
		{
			double index = 0;

			if(weighted)
			{
				//index = calculateWeightedJaccardIndex(level, region, region2);
				index = calculateJaccardIndex(level, region, region2);
			}
			else
			{
				index = calculateJaccardIndex(level, region, region2);
			}

			m_jaccardValues->at(level).at(region).push_back(index);

			//LOG(lvlDebug,QString("jaccardValue for [%1][%2] is %3").arg(region).arg(region2).arg(test));
			//LOG(lvlDebug,QString("Weighted jaccardValue for [%1][%2] is %3\n").arg(region).arg(region2).arg(m_jaccardValues->at(level).at(region).at(region2)));
		}
	}
}

//! Calculates the size of the intersection divided by the size of the union of the chosen regions
//! Gets calculated from the fiber intersection data. Value lies between 0 and 1.
//! Returns 1 if the region are the same
double iAVRMetrics::calculateJaccardIndex(int level, int region1, int region2)
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
		if(fibersInRegion2->count(fiber.first) == 1)
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
double iAVRMetrics::calculateWeightedJaccardIndex(int level, int region1, int region2)
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

	double weightedJaccard_index = sharedfibers / ((fibersOfRegion1 + fibersOfRegion2))/(sizeRegion1+ sizeRegion2);
	//double weightedJaccard_index = sharedfibers / (fibersOfRegion1 + fibersOfRegion2 - sharedfibers);

	return weightedJaccard_index;
}

//! Measures the dissimilarity (1 - jaccard index)
double iAVRMetrics::calculateJaccardDistance(int level, int region1, int region2)
{
	return 1 - calculateJaccardIndex(level, region1, region2);
}

void iAVRMetrics::calculateMaxNumberOfFibersInRegion()
{
	for (int level = 0; level < m_octrees->size(); level++)
	{
		double numberOfFibers = 0;

		for (int region = 0; region < m_fiberCoverage->at(level).size(); region++)
		{
			auto fibers = m_fiberCoverage->at(level).at(region)->size();

			if (numberOfFibers < fibers) numberOfFibers = fibers;
		}
		m_maxNumberOffibersInRegions->push_back(numberOfFibers);
	}
}

//! Calculates the histogramm bin width for two octree regions
//! Calculates further for every feature for both regions their (region)values
//! The values get stored in the HistogramParameters struct
void iAVRMetrics::calculateBinWidth(int level, std::vector<int>* featureList, int region1, int region2, std::vector<std::vector<std::vector<double>>>* regionValues)
{
	m_histogramParameter->bins = getMaxNumberOfHistogramBins(level);

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	m_histogramParameter->histogramWidth = std::vector<double>(featureList->size(), 0);
	m_histogramParameter->minValue = std::vector<double>(featureList->size(),std::numeric_limits<double>::infinity());
	m_histogramParameter->maxValue = std::vector<double>(featureList->size(), -1);

	for (int i = 0; i < featureList->size(); i++)
	{
		int feature = featureList->at(i);

		regionValues->push_back(std::vector<std::vector<double>>());

		//Region1
		regionValues->at(i).push_back(std::vector<double>());
		for (auto fiber : *fibersInRegion1)
		{
			auto value = m_objectTable->GetValue(fiber.first, feature).ToFloat();
			regionValues->at(i).at(0).push_back(value);

			if (value > m_histogramParameter->maxValue[i]) m_histogramParameter->maxValue[i] = value;
			if (value < m_histogramParameter->minValue[i]) m_histogramParameter->minValue[i] = value;
		}
	
		//Region2
		regionValues->at(i).push_back(std::vector<double>());
		for (auto fiber : *fibersInRegion2)
		{
			auto value = m_objectTable->GetValue(fiber.first, feature).ToFloat();
			regionValues->at(i).at(1).push_back(value);

			if (value > m_histogramParameter->maxValue[i]) m_histogramParameter->maxValue[i] = value;
			if (value < m_histogramParameter->minValue[i]) m_histogramParameter->minValue[i] = value;
		}
		double binWidth = (m_histogramParameter->maxValue[i] - m_histogramParameter->minValue[i]) / m_histogramParameter->bins;
		if (binWidth <= 0) binWidth = 0.0001; // If min, max is 0 -> binWidth is 0 which would yield division by 0
		m_histogramParameter->histogramWidth[i] = binWidth;
	}
}

//! Calculates how many values of each region end up in which bin.
//! The ranges of the bin are calculated like this [min,min+barWidth[ ; [min+barWidth,min+barWidth*2[ ; ... ; [min+barWidth*n,min+barWidth*(n+1)]
//! So the max value is in the last bin included (and only in the last)
void iAVRMetrics::calculateHistogramValues(int level, std::vector<int>* featureList, int region1, int region2)
{
	m_currentHistogram->clear();
	//Stores for every [feature] for both [regions] its values
	std::vector<std::vector<std::vector<double>>>* regionValues = new std::vector< std::vector<std::vector<double>>>();

	calculateBinWidth(level, featureList, region1, region2, regionValues);

	//Stores for every [feature] the occurency in every [bin]
	std::vector<int> binsInRegion1 = std::vector<int>(m_histogramParameter->bins, 0);
	std::vector<int> binsInRegion2 = std::vector<int>(m_histogramParameter->bins, 0);
	m_histogramParameter->histogramRegion1 = std::vector<std::vector<int>>(featureList->size(), binsInRegion1);
	m_histogramParameter->histogramRegion2 = std::vector<std::vector<int>>(featureList->size(), binsInRegion2);
	
	//CHANGE TO NOT RUN FROM 0 TO SIZE BUT USE VALUES IN (!!) FEATURELIST !!!
	for (int i = 0; i < featureList->size(); i++)
	{

		auto fibersInRegion1 = regionValues->at(i).at(0);
		auto fibersInRegion2 = regionValues->at(i).at(1);

		//Region1
		for (auto fiberVal : fibersInRegion1)
		{
			int bin = (int)floor((fiberVal - m_histogramParameter->minValue[i]) / m_histogramParameter->histogramWidth[i]);
			if (bin > m_histogramParameter->bins - 1) bin = bin - 1; // max value is in last bin included
			m_histogramParameter->histogramRegion1.at(i).at(bin) += 1;
		}

		//Region2
		for (auto fiberVal : fibersInRegion2)
		{
			int bin = (int)floor((fiberVal - m_histogramParameter->minValue[i]) / m_histogramParameter->histogramWidth[i]);
			if (bin > m_histogramParameter->bins - 1) bin = bin - 1;
			m_histogramParameter->histogramRegion2.at(i).at(bin) += 1;
		}
	}
}

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

#include "iAVRHistogramMetric.h"

iAVRHistogramMetric::iAVRHistogramMetric(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees) : iAVRMetrics(objectTable, io, octrees)
{
	m_currentHistogram = new std::vector<std::vector<int>>();
	m_histogramParameter = new HistogramParameters();
}

//! Calculates the number of needed bins through Sturge's Rule
int iAVRHistogramMetric::getMaxNumberOfHistogramBins(int observations)
{
	double bins = 1 + 3.322 * log(observations);
	return round(bins);
}

//! Returns all parameters of the histogram calculation of the two given regions
//! If no feature are choosen (nullptr or empty) all avaiable features are calculated
HistogramParameters* iAVRHistogramMetric::getHistogram(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2)
{
	//If no feature is choosen, calculate all
	if (featureList == nullptr || featureList->empty())
	{
		featureList = new std::vector<int>();

		for (int f = 0; f < numberOfFeatures; f++)
		{
			featureList->push_back(f);
		}
	}
	m_histogramParameter->featureList = featureList;

	calculateHistogramValues(level, maxNumberOfFibersInRegion, featureList, region1, region2);

	return m_histogramParameter;
}

//! Calculates the histogramm bin width for two octree regions
//! Calculates further for every feature for both regions their (region)values
//! The values get stored in the HistogramParameters struct
void iAVRHistogramMetric::calculateBinWidth(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2, std::vector<std::vector<std::vector<double>>>* regionValues)
{
	m_histogramParameter->bins = getMaxNumberOfHistogramBins(maxNumberOfFibersInRegion);

	auto fibersInRegion1 = m_fiberCoverage->at(level).at(region1);
	auto fibersInRegion2 = m_fiberCoverage->at(level).at(region2);

	m_histogramParameter->histogramWidth = std::vector<double>(featureList->size(), 0);
	m_histogramParameter->minValue = std::vector<double>(featureList->size(), std::numeric_limits<double>::infinity());
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
void iAVRHistogramMetric::calculateHistogramValues(int level, int maxNumberOfFibersInRegion, std::vector<int>* featureList, int region1, int region2)
{
	m_currentHistogram->clear();
	//Stores for every [feature] for both [regions] its values
	std::vector<std::vector<std::vector<double>>>* regionValues = new std::vector< std::vector<std::vector<double>>>();

	calculateBinWidth(level, maxNumberOfFibersInRegion, featureList, region1, region2, regionValues);

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
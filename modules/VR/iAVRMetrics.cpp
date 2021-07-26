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

int iAVRMetrics::numberOfFeatures = 0;
std::vector<std::vector<double>>* iAVRMetrics::m_minMaxValues = nullptr;

iAVRMetrics::iAVRMetrics(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees) : m_objectTable(objectTable), m_io(io),
m_octrees(octrees)
{
	// Initialize these values only once
	if (numberOfFeatures == 0)
	{
		//numberOfFeatures = iACsvConfig::MappedCount;
		numberOfFeatures = m_objectTable->GetNumberOfColumns();
	}
	if (m_minMaxValues == nullptr)
	{
		storeMinMaxValues();
	}
}

//! Has to be called *before* getting any Metric data
//! Sets the fiber coverage data, which is a vector for every octree level and each region, in which every fiber is stored with its coverage in that particular region. 
void iAVRMetrics::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

//! Returns the number of features stored in the .csv
int iAVRMetrics::getNumberOfFeatures()
{
	return numberOfFeatures;
}

//! Returns a string with the name of a feature from the .csv
QString iAVRMetrics::getFeatureName(int feature)
{
	//QString featureName = m_objectTable->GetColumnName(m_io.getOutputMapping()->value(feature));
	QString featureName = m_objectTable->GetColumnName(feature);
	return featureName;
}

//! Returns the min [0] and the max [1] value of a feature as vector
std::vector<double> iAVRMetrics::getMinMaxFiberValues(int feature)
{
	std::vector<double> minMax = std::vector<double>(2);
	minMax.at(0) = m_minMaxValues->at(feature).at(0);
	minMax.at(1) = m_minMaxValues->at(feature).at(1);

	return minMax;
}

//! Iterates through all values of each feature and stores the min and max value of each feature in a vector
void iAVRMetrics::storeMinMaxValues()
{
	m_minMaxValues = new std::vector<std::vector<double>>();

	std::vector<double> minAttribute = std::vector<double>();
	std::vector<double> maxAttribute = std::vector<double>();

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

//! Returns the min [0] and the max [1] value of two vectors
std::vector<double> iAVRMetrics::getMinMaxFromVec(std::vector<double> val01, std::vector<double> val02)
{
	std::vector<double> minMax = std::vector<double>(2);

	auto minMaxElem01 = std::minmax_element(val01.begin(), val01.end());
	auto minMaxElem02 = std::minmax_element(val02.begin(), val02.end());
	minMax.at(0) = vtkMath::Min(*minMaxElem01.first, *minMaxElem02.first);
	minMax.at(1) = vtkMath::Max(*minMaxElem01.second, *minMaxElem02.second);

	return minMax;
}

//! Returns the result of a value in a user given new interval between newMin and newMax
double iAVRMetrics::histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax)
{
	double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
	return result;
}

//! Returns the result of a value in a user given new interval between newMin and newMax as exponential scale
double iAVRMetrics::histogramNormalizationExpo(double value, double newMin, double newMax, double oldMin, double oldMax)
{
	newMin = log(newMin);
	newMax = log(newMax);

	double result = ((newMax - newMin) * ((value - oldMin) / (oldMax - oldMin))) + newMin;
	return exp(result);
}
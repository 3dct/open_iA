// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkType.h>

#include <vtkSmartPointer.h>

#include <QString>

class vtkTable;

class iAVROctree;

//!This class contains basic data structures/values for metrics calculation
class iAVRMetrics
{
public:
	iAVRMetrics(vtkTable* objectTable, std::vector<iAVROctree*> const & octrees);
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * fiberCoverage);
	vtkIdType getNumberOfFeatures();
	QString getFeatureName(vtkIdType feature);

	static std::vector<double> getMinMaxFromVec(std::vector<double> val01, std::vector<double> val02);
	static double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);
	static double histogramNormalizationExpo(double value, double newMin, double newMax, double oldMin, double oldMax);

protected:
	static vtkIdType numberOfFeatures;
	//Stores the for a [feature] the [0] min and the [1] max value from the csv file
	static std::vector<std::vector<double>>* m_minMaxValues;

	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * m_fiberCoverage;
	vtkSmartPointer<vtkTable> m_objectTable;
	std::vector<iAVROctree*> const & m_octrees;

private:
	void storeMinMaxValues();
};

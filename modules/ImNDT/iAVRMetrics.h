// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvIO.h"
#include "iAVROctree.h"
#include "iAVR3DText.h"

#include "vtkTable.h"

#include <QString>

//!This class contains basic data structures/values for metrics calculation
class iAVRMetrics
{
public:
	iAVRMetrics(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage);
	int getNumberOfFeatures();
	QString getFeatureName(int feature);
	std::vector<double> getMinMaxFiberValues(int feature);

	static std::vector<double> getMinMaxFromVec(std::vector<double> val01, std::vector<double> val02);
	static double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);
	static double histogramNormalizationExpo(double value, double newMin, double newMax, double oldMin, double oldMax);

protected:
	static int numberOfFeatures;
	//Stores the for a [feature] the [0] min and the [1] max value from the csv file
	static std::vector<std::vector<double>>* m_minMaxValues;

	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;
	iACsvIO m_io;
	vtkSmartPointer<vtkTable> m_objectTable;
	std::vector<iAVROctree*>* m_octrees;

private:
	void storeMinMaxValues();
};

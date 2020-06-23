/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <vtkSmartPointer.h>
#include <unordered_map>

#include <QString>

#include "iACsvIO.h"
#include "iAVROctree.h"

#include "vtkTable.h"
#include "vtkLookupTable.h"
#include "vtkScalarBarActor.h"
#include "vtkRenderer.h"

//!This class calculates the metrics used in the Model in Miniature Heatmap
class iAVRMetrics
{
public:
	iAVRMetrics(vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage);
	std::vector<std::vector<double>>* getHeatmapColoring(int octreeLevel, int feature);
	vtkSmartPointer<vtkLookupTable> getLut();
	vtkSmartPointer<vtkScalarBarActor> getColorBarLegend();
	void showColorBarLegend(vtkRenderer* ren);
	void hideColorBarLegend(vtkRenderer* ren);
	int getNumberOfFeatures();
	QString getFeatureName(int feature);
	

private:
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;
	//Stores for an [octree level] for choosen [feature] for every [octree region] the metric value
	std::vector<std::vector<std::vector<double>>>* m_calculatedStatistic;
	//Stores the for a [feature] the [0] min and the [1] max value from the csv file
	std::vector<std::vector<double>>* m_minMaxValues;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkScalarBarActor> m_colorBar;
	iACsvIO m_io;
	bool m_colorBarVisible;
	std::vector<iAVROctree*>* m_octrees;
	//Stores the info if at a specific octree [level] a specific [feature] is already calculated
	std::vector<std::vector<bool>>* isAlreadyCalculated;
	int numberOfFeatures;

	void calculateWeightedAverage(int octreeLevel, int feature);
	double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);
	vtkSmartPointer<vtkLookupTable> calculateLUT(double min, double max, int tableSize);
	void storeMinMaxValues();

};
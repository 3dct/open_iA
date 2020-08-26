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
#include <thread>

#include <QString>

#include "iACsvIO.h"
#include "iAVROctree.h"
#include "iAVR3DText.h"

#include "vtkTable.h"
#include "vtkLookupTable.h"
#include "vtkActor.h"
#include "vtkScalarBarActor.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTextActor3D.h"

struct HistogramParameters
{
	//featureList needed to map from array position back to feature id
	std::vector<int>* featureList;
	//Stores for every [feature] the min value of both regions
	std::vector<double> minValue;
	//Stores for every [feature] the max value of both regions
	std::vector<double> maxValue;
	//Stores for every [feature] the calculated bin width
	std::vector<double> histogramWidth;
	//Amount of bins for current histogram
	int bins;
	//Stores for every [feature] the occurency in every [bin]
	std::vector<std::vector<int>> histogramRegion1;
	std::vector<std::vector<int>> histogramRegion2;
};

//!This class calculates the metrics used in the Model in Miniature Heatmap
class iAVRMetrics
{
public:
	iAVRMetrics(vtkRenderer* renderer, vtkTable* objectTable, iACsvIO io, std::vector<iAVROctree*>* octrees);
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage);
	std::vector<QColor>* getHeatmapColoring(int octreeLevel, int feature, int valueMapping);
	vtkSmartPointer<vtkLookupTable> getLut();
	std::vector<std::vector<std::vector<vtkIdType>>>* getMaxCoverageFiberPerRegion();
	void calculateColorBarLegend();
	void showColorBarLegend();
	void hideColorBarLegend();
	int getNumberOfFeatures();
	QString getFeatureName(int feature);
	void moveColorBarLegend(double *pos);
	void rotateColorBarLegend(double x, double y, double z);
	void setLegendTitle(QString title);
	std::vector<std::vector<std::vector<double>>>* getWeightedJaccardIndex(int level);
	void createMIPPanels(int octreeLevel, int feature);
	void createSingleMIPPanel(int octreeLevel, int feature, int viewDir);
	void hideMIPPanels();
	double getMaxNumberOfFibersInRegion(int level);
	int getMaxNumberOfHistogramBins(int level);
	HistogramParameters* getHistogram(int level, std::vector<int>* featureList, int region1, int region2);

	static double histogramNormalization(double value, double newMin, double newMax, double oldMin, double oldMax);
	static double histogramNormalizationExpo(double value, double newMin, double newMax, double oldMin, double oldMax);

private:
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;
	//Stores for the [octree level] in an [octree region] the fibers which have the max coverage (Every Fiber can only be in one region)
	std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage;
	//Stores for an [octree level] for choosen [feature] for every [octree region] the metric value
	std::vector<std::vector<std::vector<double>>>* m_calculatedStatistic;
	//Stores the for a [feature] the [0] min and the [1] max value from the csv file
	std::vector<std::vector<double>>* m_minMaxValues;
	//Stores for the [octree level] in an [octree region] its Jaccard index to another [octree region]
	std::vector<std::vector<std::vector<double>>>* m_jaccardValues;
	//Stores for the [octree level] the max amount of fibers which lie in an octree region
	std::vector<double>* m_maxNumberOffibersInRegions;
	//Stores the minValue, maxValue, binWidth and bin amount
	HistogramParameters* m_histogramParameter;
	//Stores the currently calculated histogram for the two [regions] and their [bins] with the cumulative number of observations 
	std::vector<std::vector<int>>* m_currentHistogram;
	vtkSmartPointer<vtkTable> m_objectTable;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkActor> m_ColorBar;
	vtkSmartPointer<vtkTextActor3D> textSource;
	vtkSmartPointer<vtkTextActor3D> titleTextSource;
	vtkSmartPointer<vtkActor> mipPanel;
	std::vector<vtkPolyData*> mipPlanes;
	vtkSmartPointer<vtkRenderer> m_renderer;
	iACsvIO m_io;
	std::vector<iAVR3DText*>* m_3DLabels;
	bool m_colorBarVisible;
	std::vector<iAVROctree*>* m_octrees;
	//Stores the info if at a specific octree [level] a specific [feature] is already calculated
	std::vector<std::vector<bool>>* isAlreadyCalculated;
	int numberOfFeatures;
	bool m_maxCoverageisAlreadyCalculated;

	void calculateWeightedAverage(int octreeLevel, int feature);
	vtkSmartPointer<vtkLookupTable> calculateLUT(double min, double max, int tableSize);
	void storeMinMaxValues();
	void calculateMaxCoverageFiberPerRegion();
	void findBiggestCoverage(int level, int fiber);
	void calculateJaccardIndex(int level, bool weighted);
	double calculateJaccardIndex(int level, int region1, int region2);
	double calculateWeightedJaccardIndex(int level, int region1, int region2);
	double calculateJaccardDistance(int level, int region1, int region2);
	std::vector<QColor>* calculateMIPColoring(int direction, int level, int feature);
	void calculateMaxNumberOfFibersInRegion();
	void calculateBinWidth(int level, std::vector<int>* featureList, int region1, int region2, std::vector<std::vector<std::vector<double>>>* regionValues);
	void calculateHistogramValues(int level, std::vector<int>* featureList, int region1, int region2);
};
// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompHistogramCalculation.h"
#include "iACompVisOptions.h"

#include <map>

class iACsvDataStorage;
class iACompHistogramTableData;
class iACompHistogramVis;
class iACompUniformBinningData;
class iACompVisMain;

class iAMainWindow;

class iACompHistogramTable
{

   public:
	/************************** Setter & Getter ***************************************/
	iACsvDataStorage* getDataStorage();
	void setDataStorage(iACsvDataStorage* storage);
	iACompHistogramVis* getHistogramTableVis();

	iACompUniformBinningData* getUniformBinningData();
	iACompBayesianBlocksData* getBayesianBlocksData();
	iACompNaturalBreaksData* getNaturalBreaksData();
	iACompKernelDensityEstimationData* getKernelDensityEstimationData();

	/**************************  Change Table Visualization Methods  ******************************/
	void drawUniformTable();
	void drawBayesianBlocksTable();
	void drawNaturalBreaksTable();
	void drawCurveTable();
	void drawWhiteCurveTable();

	/******************************************  Update  ******************************/
	void resetOtherCharts();
	void updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);

	//draw the datasets with rows ordered ascending to its amount of objects
	void drawDatasetsInAscendingOrder();
	//draw the datasets with rows ordered descending to its amount of objects
	void drawDatasetsInDescendingOrder();
	//draw the datasets with rows ordered according to loading the datasets
	void drawDatasetsInOriginalOrder();
	//deactivates the ordering button in the menu
	void deactivateOrderingButton();
	//activates the ordering button in the menu
	void activateOrderingButton();

	iACompHistogramTableData* recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins);
	iACompHistogramTableData* calculateSpecificBins(
		iACompVisOptions::binningType binningType, bin::BinType* data, int currBin, int amountOfBins);
	iACompKernelDensityEstimationData* recomputeKernelDensityCurveUB();

	iACompHistogramTable(
		iAMainWindow* parent, iACsvDataStorage* m_dataStorage, iACompVisMain* main, bool MDSComputedFlag);

	void reinitializeHistogramTable();

	std::vector<int>* getAmountObjectsEveryDataset();

   private:

	   //initialize the various binning methods
	   void initializeBinCalculation(bool mdsComputedFlag);

		iACompVisMain* m_main;
		iACsvDataStorage* m_dataStorage;

		//holds the data for which the MDS will be calculated
		//list containing all csv-files
		//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
		//header = [name1,name2,...] --> Strings
		//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]
		QList<csvFileData>* m_inputData;

		iACompHistogramCalculation* histogramCalculation;
		iACompHistogramVis* histogramVis;

		int m_amountDatasets;
};

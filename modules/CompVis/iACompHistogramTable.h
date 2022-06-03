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
#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
#include "iACompHistogramTableData.h"
#include "iACompHistogramCalculation.h"
#include "iACompHistogramVis.h"
#include "iACompVisOptions.h"

class iAMainWindow;
class iACompVisMain;
class iACompUniformBinningData;

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


	//iACompHistogramTable(iAMainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* m_dataStorage, iACompVisMain* main);
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

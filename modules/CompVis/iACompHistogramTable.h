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

	/******************************************  Update  ******************************/
	void resetOtherCharts();
	void updateOtherCharts(csvDataType::ArrayType* selectedData, std::map<int, std::vector<double>>* pickStatistic);

	//draw the datasets with rows ordered ascending to its amount of objects
	void drawDatasetsInAscendingOrder();
	//draw the datasets with rows ordered descending to its amount of objects
	void drawDatasetsInDescendingOrder();
	//draw the datasets with rows ordered according to loading the datasets
	void drawDatasetsInOriginalOrder();

	iACompHistogramTableData* recalculateBinning(iACompVisOptions::binningType binningType, int numberOfBins);
	iACompHistogramTableData* calculateSpecificBins(
		iACompVisOptions::binningType binningType, bin::BinType* data, int currBin, int amountOfBins);


	//iACompHistogramTable(iAMainWindow* parent, iAMultidimensionalScaling* mds, iACsvDataStorage* m_dataStorage, iACompVisMain* main);
	iACompHistogramTable(
		iAMainWindow* parent, iACsvDataStorage* m_dataStorage, iACompVisMain* main, bool MDSComputedFlag);

	void reinitializeHistogramTable(iAMultidimensionalScaling* newMds);

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
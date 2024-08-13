// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompUniformBinning.h"
#include "iACompBayesianBlocks.h"
#include "iACompNaturalBreaks.h"
#include "iACompKernelDensityEstimation.h"

class iACsvDataStorage;
class iACompUniformBinningData;
class iACompBayesianBlocksData;
class iACompNaturalBreaksData;
class iACompKernelDensityEstimationData;

class iACompHistogramCalculation
{

public:
	iACompHistogramCalculation(iACsvDataStorage* dataStorage, bool mdsComputed);

	/************************** Uniform Binning ***************************************/
	iACompUniformBinningData* getUniformBinningData();

	//calculates the uniform binning with 10 bins
	void calculateUniformBinning();
	//calculates the uniform binning with the number stored in the variable "numberOfBins" bins
	void calculateUniformBinning(int numberOfBins);
	void calculateUniformBinningSpecificBins(bin::BinType* data, int currData, int numberOfBins);

	/************************** Bayesian Blocks ***************************************/
	iACompBayesianBlocksData* getBayesianBlocksData();

	void calculateBayesianBlocks();


	/************************** Jenks Natural Breaks ***************************************/
	iACompNaturalBreaksData* getNaturalBreaksData();

	void calculateNaturalBreaks();

	/************************** DBScan Clustering ***************************************/
	//TODO

	/************************** Kernel Density Estimation ***************************************/
	iACompKernelDensityEstimationData* getKernelDensityEstimationData();
	void calculateDensityEstimation();
	void recalculateDensityEstimationUniformBinning();

	//add other binning techniques here

private:

	void orderDataPointsByDatasetAffiliation(std::vector<double> const * histbinlist);


	iACsvDataStorage* m_dataStorage;

	//vector that stores the number of elements for every dataset
	//i.e. dataset_1 stores 10 objects...
	std::vector<int>* m_amountObjectsEveryDataset;
	//array where the size of the rows is not always the same
	bin::BinType* m_datasets;


	double m_maxVal;
	double m_minVal;

	iACompUniformBinningData* m_uniformBinningData;
	iACompUniformBinning* m_uniformBinning;


	iACompBayesianBlocksData* m_bayesianBlocksData;
	iACompBayesianBlocks* m_bayesianBlocks;

	iACompNaturalBreaksData* m_naturalBreaksData;
	iACompNaturalBreaks* m_naturalBreaks;

	iACompKernelDensityEstimationData* m_densityEstimationData;
	iACompKernelDensityEstimation* m_densityEstimation;
};

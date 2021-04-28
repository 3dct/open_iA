#pragma once

#include "iACompUniformBinning.h";



class iAMultidimensionalScaling;
class iACsvDataStorage;
class iACompUniformBinningData;


class iACompHistogramCalculation
{

public:
	iACompHistogramCalculation(iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage);

	iACompUniformBinningData* getUniformBinningData();

	//calculates the uniform binning with 10 bins
	void calculateUniformBinning();
	//calculates the uniform binning with the number stored in the variable "numberOfBins" bins
	void calculateUniformBinning(int numberOfBins);
	void calculateUniformBinningSpecificBins(bin::BinType* data, int currData, int numberOfBins);

	//TODO add other binning techniques

private:

	void orderDataPointsByDatasetAffiliation(std::vector<double>* histbinlist);

	//conatins the calcuation of the Multdimensional Scaling
	iAMultidimensionalScaling* m_mds;
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

};

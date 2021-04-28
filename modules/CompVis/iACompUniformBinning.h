#pragma once

#include "iACompBinning.h"

class iACompUniformBinningData;

class iACompUniformBinning : public iACompBinning
{
public:

	iACompUniformBinning(iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets);

	//calculate a uniform binning for the data points (there is always the same amount of bins for each dataset)
	virtual void calculateBins();

	//calculates the bin datastructure for (a) specifically selected bin(s)
	virtual bin::BinType* calculateBins(bin::BinType* data, int currData);

	

	virtual void setDataStructure(iACompHistogramTableData* datastore);
	//set how many bins should be computed for each dataset - has to be set before calling calculateBins()!
	void setCurrentNumberOfBins(int currentNumberOfBins);


	int getMaxAmountInAllBins();


private:

	//calculates the maximum number of elements in a bin (over all bins)
	void initializeMaxAmountInBins(bin::BinType* bins, int initialNumberBins);
	
/*	//amount of bins in the histogram for all rows/datasets
	int m_bins;
*/
	//maximum amount of numbers in a bin (calculated for all bins)
	int m_maxAmountInAllBins;


	iACompUniformBinningData* m_uniformBinningData;

	//amount of bins that will be calculated for the next histogram
	int m_currentNumberOfBins;
};

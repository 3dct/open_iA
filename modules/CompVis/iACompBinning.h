#pragma once

//CompVis
#include "iACompHistogramTableData.h"

//Qt
#include <qlist.h>

class iACompBinning
{

public:

	iACompBinning(iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets);

	//calculate the binning for the data points
	virtual void calculateBins() = 0;
	
	//calculates the bin datastructure for (a) specifically selected bin(s)
	virtual bin::BinType* calculateBins(bin::BinType* data, int currData) = 0;

	virtual void setDataStructure(iACompHistogramTableData* datastructure) = 0;

protected:

	//checks if the value lies inside an interval [low,high[
	bool checkRange(double value, double low, double high);
	
	//array where the size of the rows is not always the same
	bin::BinType* m_datasets;

	iACsvDataStorage* m_dataStorage;

};

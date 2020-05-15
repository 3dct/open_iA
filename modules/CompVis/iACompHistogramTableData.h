#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"

//Qt
#include <qlist.h>

struct bin
{
	//BinType corresponds to a datastructure and can varying row length for each column and vice versa
	//ArrayType != BinType
	using BinType = std::vector<std::vector<double>>;

	static BinType* initialize(int amountBins);
	static void debugBinType(BinType* input);
	
};

//data structure used to draw the histogram table
class iACompHistogramTableData
{
   public:
	iACompHistogramTableData(iAMultidimensionalScaling* mds);
	QList<bin::BinType*>* getBinData();
	double getMaxVal();
	double getMinVal();
	int getMaxAmountInAllBins();

   private:
	void calculateBins();
	bool checkRange(double value, double low, double high);
	void initializeMaxAmountInBins(bin::BinType* bins);

	iAMultidimensionalScaling* m_mds;
	double m_maxVal;
	double m_minVal;
	int m_bins;
	int m_maxAmountInAllBins;

	std::vector<int>* amountObjectsEveryDataset;
	//array where the size of the rows is not always the same
	csvDataType::ArrayType* datasets;
	//stores the bin data for all datasets
	QList<bin::BinType*>* binData;
};
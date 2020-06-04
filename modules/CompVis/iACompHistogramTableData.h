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

	//initialize with a certain amount of bins
	static BinType* initialize(int amountBins);
	//creates a deep copy of the list of bintypes
	static QList<bin::BinType*>* DeepCopy(QList<bin::BinType*>* input);
	//outputs the content of the bintype
	static void debugBinType(BinType* input);
	
};

//data structure used to draw the histogram table
class iACompHistogramTableData
{
   public:
	iACompHistogramTableData(iAMultidimensionalScaling* mds);
	//returns the bin datastructure for all datasets
	QList<bin::BinType*>* getBinData();
	//returns the value of the maximum value in the whole dataset
	double getMaxVal();
	//returns the value of the minimum value in the whole dataset
	double getMinVal();
	//returns the maximum amount of numbers in all bins --> i.e. there are maximum 5 values in one bin
	int getMaxAmountInAllBins();
	
	//calcualtes the bin datastructure for all datasets/rows
	QList<bin::BinType*>* calculateBins(int numberOfBins);
	//calculates the bin datastructure for (a) specifically selected bin(s)
	bin::BinType* calculateBins(bin::BinType* data, int currData, int numberOfBins);

   private:
	//checks if the value lies inside an interval [low,high[
	bool checkRange(double value, double low, double high);
	//calculates the maximum number of elements in a bin (over all bins)
	void initializeMaxAmountInBins(bin::BinType* bins);

	//conatins the calcuation of the Multdimensional Scaling
	iAMultidimensionalScaling* m_mds;
	//maximum value in all datasets
	double m_maxVal;
	//minimum value in all datasets
	double m_minVal;
	//amount of bins in the histogram for all rows/datasets
	int m_bins;
	//maximum amount of numbers in a bin (calculated for all bins)
	int m_maxAmountInAllBins;

	//vector that stores the number of elements for every dataset
	//i.e. dataset_1 stores 10 objects...
	std::vector<int>* amountObjectsEveryDataset;
	//array where the size of the rows is not always the same
	csvDataType::ArrayType* datasets;
	//stores the bin data for all datasets
	QList<bin::BinType*>* binData;
};
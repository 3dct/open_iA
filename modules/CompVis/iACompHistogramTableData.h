#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "vtkIdTypeArray.h"
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
	//copies the specified cells into a new BinType object
	static bin::BinType* copyCells(bin::BinType* input, std::vector<vtkIdType>* indexOfCellsToCopy);
	
};

//data structure used to draw the histogram table
class iACompHistogramTableData
{
   public:

	iACompHistogramTableData();
	
	//returns the value of the maximum value in the whole dataset
	double getMaxVal();
	//returns the value of the minimum value in the whole dataset
	double getMinVal();
	

	//returns the bin datastructure for all datasets
	QList<bin::BinType*>* getBinData();

	//get the fibers stored per bin per dataset/(row)
	QList<std::vector<csvDataType::ArrayType*>*>* getObjectsPerBin();

	std::vector<int>* getAmountObjectsEveryDataset();

	bin::BinType* getZoomedBinData();

	void setMaxVal(double newMax);
	void setMinVal(double newMin);
	void setBinData(QList<bin::BinType*>* newBinData);
	void setBinDataObjects(QList<std::vector<csvDataType::ArrayType*>*>* newBinDataObjects);
	void setAmountObjectsEveryDataset(std::vector<int>* newAmountObjectsEveryDataset);
	void setZoomedBinData(bin::BinType* newZoomedBinData);

	void debugBinDataObjects();

   protected:
	
	//maximum value in all datasets
	double m_maxVal;
	//minimum value in all datasets
	double m_minVal;
	

	//vector that stores the number of elements for every dataset
	//i.e. dataset_1 stores 10 objects...
	std::vector<int>* amountObjectsEveryDataset;
	
	//stores the bin data for all datasets
	//contains the values of the MDS
	QList<bin::BinType*>* binData;

	//stores selected data points, which have been divided into bins again
	bin::BinType* zoomedBinData;
	

	//stores the fiber ids for all datasets according to their bin
	//contains the attributes of the selected objects (fibers,...) like Id,...
	QList<std::vector<csvDataType::ArrayType*>*>* binDataObjects;
};
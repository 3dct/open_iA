// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "vtkIdTypeArray.h"
#include "iAMultidimensionalScaling.h"

//Qt
#include <qlist.h>

class vtkPolyData;

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
	//returns the minimum and maximum value contained in the BinyType object
	static std::vector<double>* getMinimumAndMaximum(bin::BinType* input);
	
};

//data structure used to draw the histogram table
class iACompHistogramTableData
{
   public:

	iACompHistogramTableData();

	//returns for every dataset each bin defined by its lower boundary
	virtual QList<std::vector<double>>* getBinBoundaries() = 0;
	//set for each dataset for each bin its lowerBoundary
	virtual void setBinBoundaries(QList<std::vector<double>>* binBoundaries) = 0;

	//get the for each dataset for each bin the number of objects contained
	virtual QList<std::vector<double>>* getNumberOfObjectsPerBinAllDatasets();
	
	//returns the value of the maximum value in the whole dataset
	double getMaxVal();
	//returns the value of the minimum value in the whole dataset
	double getMinVal();

	//returns the bin datastructure for all datasets storing the MDS values
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

	//returns the maximum amount of numbers in all bins --> i.e. there are maximum 5 values in one bin
	int getMaxAmountInAllBins();
	void setMaxAmountInAllBins(int newMaxAmountInAllBins);

	void debugBinDataObjects();

	/*** Rendering Information***/
	//store vtkPolyData that stores the drawn bin borders of a single dataset
	void storeBinPolyData(vtkSmartPointer<vtkPolyData> newBinPolyData);
	//reset the store of the bin boundaries
	void resetBinPolyData();
	QList<vtkSmartPointer<vtkPolyData>>* getBinPolyData();

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

	//maximum amount of numbers in a bin (calculated for all bins)
	//calculated through the uniform binning appraoch!
	int m_maxAmountInAllBins;

	//stores for each dataset the lower boundary of each bin
	QList<std::vector<double>>* m_binsBoundaries;

	//stores the borders of the bins of each dataset
	QList<vtkSmartPointer<vtkPolyData>>* m_binPolyDatasets;
};

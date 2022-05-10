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
#include "iACompHistogramTableData.h"

//CompVis
#include "iACsvDataStorage.h"

#include <vector>

iACompHistogramTableData::iACompHistogramTableData(iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage) :
	m_mds(mds),
	m_dataStorage(dataStorage),
	m_bins(10)
{
	std::vector<double>* histbinlist = csvDataType::arrayTypeToVector(m_mds->getResultMatrix());
	auto result = std::minmax_element(histbinlist->begin(), histbinlist->end());

	m_maxVal = *result.second;
	m_minVal = *result.first;

	datasets = new bin::BinType();
	amountObjectsEveryDataset = csvFileData::getAmountObjectsEveryDataset(m_mds->getCSVFileData());
	
	int add = 0;
	for (size_t i = 0; i < amountObjectsEveryDataset->size(); ++i)
	{ 
		std::vector<double>::const_iterator first = histbinlist->begin() + add;
		add += amountObjectsEveryDataset->at(i);
		std::vector<double>::const_iterator last = histbinlist->begin() + add;

		std::vector<double> segment(first, last);
		datasets->push_back(segment);
	}

	calculateBins(m_bins);
}

QList<bin::BinType*>* iACompHistogramTableData::calculateBins(int numberOfBins)
{

	binData = new QList<bin::BinType*>; //stores MDS values
	binDataObjects = new QList<std::vector<csvDataType::ArrayType*>*>; //stores data of selected objects attributes


	double length = std::abs(m_maxVal) + std::abs(m_minVal);
	double binLength = length / numberOfBins;

	for (size_t i = 0; i < amountObjectsEveryDataset->size(); ++i)
	{// do for every dataset

		std::vector<double> values = datasets->at(i);
		bin::BinType* bins = bin::initialize(numberOfBins);

		//initalize
		std::vector<csvDataType::ArrayType*>* binsWithFiberIds = new std::vector<csvDataType::ArrayType*>();
		for(int j= 0; j < numberOfBins; ++j)
		{
			csvDataType::ArrayType* init = new csvDataType::ArrayType();
			binsWithFiberIds->push_back(init);
		}

		int datasetInd = values.size();
	
		//check for every value inside a dataset for the corresponding bin
		for (size_t v = 0; v < values.size(); ++v)
		{
			for (int b = 0; b < numberOfBins; ++b)
			{
				bool inside = checkRange(values.at(v), m_minVal + (binLength * b), m_minVal + (binLength * (b+1)));

				if (!inside && b == numberOfBins - 1)
				{
					inside = (abs(m_maxVal - values.at(v)) < 0.0000001);
				}
				if (inside)
				{
					//store MDS value
					bins->at(b).push_back(values.at(v));

					//store Fiber ID
					std::vector<double> object = m_dataStorage->getData()->at(i).values->at(v);//.at(0);
					csvDataType::ArrayType* data = binsWithFiberIds->at(b);
					data->push_back(object);
					//binsWithFiberIds->at(b) = data;

					//LOG(lvlDebug,"fibers stored = " + QString::number(data->size()) + " --> at Bin: " + QString::number(b));

					break;
				}
			}

			datasetInd--;
		}

		
		initializeMaxAmountInBins(bins);
		binData->push_back(bins);
		binDataObjects->push_back(binsWithFiberIds);
	}

	/*LOG(lvlDebug,"");
	//DEBUG
	for(int i = 0; i < binDataObjects->size(); i++)
	{ //datasets
		for(int k = 0; k < binDataObjects->at(i)->size(); k++)
		{ //bins

			csvDataType::ArrayType* data = binDataObjects->at(i)->at(k);

			for(int j = 0; j < data->size(); j++)
			{
				LOG(lvlDebug,"fiberLabelId = " + QString::number(data->at(j).at(0)) + " --> at Bin: " + QString::number(k));
			}
			
		}
	}
	LOG(lvlDebug,"");
	LOG(lvlDebug,"#######################################################");
	*/
	return binData;
}

bin::BinType* iACompHistogramTableData::calculateBins(bin::BinType* data, int currData, int numberOfBins)
{	
	size_t amountVals = data->at(currData).size();

	if (amountVals == 0)
	{
		return nullptr;
	}

	std::vector<double> vals = data->at(currData);

	auto result = std::minmax_element(vals.begin(), vals.end());
	double min = *result.first;
	double max = *result.second;

	double length = max - min;
	double binLength = length / numberOfBins;

	bin::BinType* bins = bin::initialize(numberOfBins);


	for (size_t v = 0; v < amountVals; v++)
	{
		for (int b = 0; b < numberOfBins; b++)
		{
			bool inside = checkRange(vals.at(v), min + (binLength * b), min + (binLength * (b + 1)));
			
			//check if the last value is the maximum value
			//otherwise this value would never be added to a bin
			if (!inside && b == numberOfBins - 1)
			{
				inside = (vals.at(v) == max);
			}

			if (inside)
			{
				bins->at(b).push_back(vals.at(v));
				break;
			}
		}
	}

	return bins;
}

bool iACompHistogramTableData::checkRange(double value, double low, double high)
{
	return ((value >= low) && (value < high));
}

void iACompHistogramTableData::initializeMaxAmountInBins(bin::BinType* bins)
{
	for (int ind = 0; ind < m_bins; ind++) 
	{
		int size = bins->at(ind).size();
		if (m_maxAmountInAllBins < size)
		{
			m_maxAmountInAllBins = size;
		}
	}
}

QList<bin::BinType*>* iACompHistogramTableData::getBinData()
{
	return binData;
}

double iACompHistogramTableData::getMaxVal()
{
	return m_maxVal;
}

double iACompHistogramTableData::getMinVal()
{
	return m_minVal;
}

int iACompHistogramTableData::getMaxAmountInAllBins()
{
	return m_maxAmountInAllBins;
}

QList<std::vector<csvDataType::ArrayType*>*>* iACompHistogramTableData::getObjectsPerBin()
{
	return binDataObjects;
}


/************************** bin methods ***************************************/

bin::BinType* bin::initialize(int amountBins)
{
	return new bin::BinType(amountBins);
}

void bin::debugBinType(BinType* input)
{
	const size_t amountCols = input->size();

	//DEBUG
	LOG(lvlDebug," ");
	LOG(lvlDebug,"Bins: " + QString::number(amountCols));
	LOG(lvlDebug,"Bin Matrix: ");
	for (size_t col1 = 0; col1 < amountCols; ++col1)
	{
		LOG(lvlDebug,"Bin " + QString::number(col1) + ":");
		for (size_t r1 = 0; r1 < input->at(col1).size(); ++r1)
		{
			LOG(lvlDebug,"  Values " + QString::number(r1) + ": " + QString::number(input->at(col1).at(r1)));
		}
	}
	LOG(lvlDebug," ");
}

QList<bin::BinType*>* bin::DeepCopy(QList<bin::BinType*>* input)
{
	QList<bin::BinType*>* output = new QList<bin::BinType*>();

	for (int binInd = 0; binInd < input->size(); binInd++) 
	{
		bin::BinType* curBin = input->at(binInd);
		bin::BinType* newBin = initialize(curBin->size());

		for (size_t indVals = 0; indVals < curBin->size(); ++indVals)
		{
			newBin->at(indVals) = curBin->at(indVals);
		}

		output->append(newBin);
	}

	return output;
}

bin::BinType* bin::copyCells(bin::BinType* input, std::vector<vtkIdType>* indexOfCellsToCopy)
{
	bin::BinType* output = new bin::BinType();

	for(size_t i = 0; i < indexOfCellsToCopy->size(); ++i)
	{
		output->push_back( input->at(indexOfCellsToCopy->at(i)) );
	}

	return output;
}
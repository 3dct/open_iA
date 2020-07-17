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
	for (int i = 0; i < amountObjectsEveryDataset->size(); i++)
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
	//DEBUG_LOG("");

	binData = new QList<bin::BinType*>; //stores MDS values
	binDataObjects = new QList<std::vector<csvDataType::ArrayType*>*>; //stores data of selected objects attributes

	double length = std::abs(m_maxVal) + std::abs(m_minVal);
	double binLength = length / numberOfBins;

	for (int i = 0; i < amountObjectsEveryDataset->size(); i++)
	{// do for every dataset

		std::vector<double> values = datasets->at(i);
		bin::BinType* bins = bin::initialize(numberOfBins);

		//initalize
		std::vector<csvDataType::ArrayType*>* binsWithFiberIds = new std::vector<csvDataType::ArrayType*>();
		for(int i= 0; i < numberOfBins; i++)
		{
			csvDataType::ArrayType* init = new csvDataType::ArrayType();
			binsWithFiberIds->push_back(init);
		}
		

		int datasetInd = values.size();
	
		//check for every value inside a dataset for the corresponding bin
		for (int v = 0; v < values.size(); v++)
		{
			for (int b = 0; b < numberOfBins; b++)
			{
				bool inside = checkRange(values.at(v), m_minVal + (binLength * b), m_minVal + (binLength * (b+1)));
				if (!inside && b == numberOfBins - 1)
				{
					inside = (values.at(v) == m_maxVal);
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

					//DEBUG_LOG("fibers stored = " + QString::number(data->size()) + " --> at Bin: " + QString::number(b));

					break;
				}
			}

			datasetInd--;
		}

		
		initializeMaxAmountInBins(bins);
		binData->push_back(bins);
		binDataObjects->push_back(binsWithFiberIds);
	}

	/*DEBUG_LOG("");
	//DEBUG
	for(int i = 0; i < binDataObjects->size(); i++)
	{ //datasets
		for(int k = 0; k < binDataObjects->at(i)->size(); k++)
		{ //bins

			csvDataType::ArrayType* data = binDataObjects->at(i)->at(k);

			for(int j = 0; j < data->size(); j++)
			{
				DEBUG_LOG("fiberLabelId = " + QString::number(data->at(j).at(0)) + " --> at Bin: " + QString::number(k));
			}
			
		}
	}
	DEBUG_LOG("");
	DEBUG_LOG("#######################################################");
	*/
	return binData;
}

bin::BinType* iACompHistogramTableData::calculateBins(bin::BinType* data, int currData, int numberOfBins)
{	
	int amountVals = data->at(currData).size();

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


	for (int v = 0; v < amountVals; v++)
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
	int amountCols = input->size();

	//DEBUG
	DEBUG_LOG(" ");
	DEBUG_LOG("Bins: " + QString::number(amountCols));
	DEBUG_LOG("Bin Matrix: ");
	for (int col1 = 0; col1 < amountCols; col1++)
	{
		DEBUG_LOG("Bin " + QString::number(col1) + ":");
		for (int r1 = 0; r1 < input->at(col1).size(); r1++)
		{
			DEBUG_LOG("  Values " + QString::number(r1) + ": " + QString::number(input->at(col1).at(r1)));
		}
	}
	DEBUG_LOG(" ");
}

QList<bin::BinType*>* bin::DeepCopy(QList<bin::BinType*>* input)
{
	QList<bin::BinType*>* output = new QList<bin::BinType*>();

	for (int binInd = 0; binInd < input->size(); binInd++) 
	{
		bin::BinType* curBin = input->at(binInd);
		bin::BinType* newBin = initialize(curBin->size());

		for (int indVals = 0; indVals < curBin->size(); indVals++)
		{
			newBin->at(indVals) = (curBin->at(indVals));
		}

		output->append(newBin);
	}

	return output;
}
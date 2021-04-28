#include "iACompUniformBinning.h"

#include "iACompUniformBinningData.h"

iACompUniformBinning::iACompUniformBinning(iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets) :
	iACompBinning(dataStorage, amountObjectsEveryDataset, datasets), 
	m_uniformBinningData(nullptr), 
	m_currentNumberOfBins(-1)
{
}

void iACompUniformBinning::setDataStructure(iACompHistogramTableData* datastore)
{
	m_uniformBinningData = static_cast<iACompUniformBinningData*>(datastore);
}

void iACompUniformBinning::calculateBins()
{

	QList<bin::BinType*>*  binData = new QList<bin::BinType*>;          //stores MDS values
	QList<std::vector<csvDataType::ArrayType*>*>* binDataObjects = new QList<std::vector<csvDataType::ArrayType*>*>;  //stores data of selected objects attributes

	double initialNumberBins = m_uniformBinningData->getInitialNumberOfBins();
	double minVal = m_uniformBinningData->getMinVal();
	double maxVal = m_uniformBinningData->getMaxVal();

	double length = std::abs(maxVal) + std::abs(minVal);
	double binLength = length / m_currentNumberOfBins;

	for (int i = 0; i < m_uniformBinningData->getAmountObjectsEveryDataset()->size(); i++)
	{// do for every dataset

		std::vector<double> values = m_datasets->at(i);
		bin::BinType* bins = bin::initialize(m_currentNumberOfBins);

		//initalize
		std::vector<csvDataType::ArrayType*>* binsWithFiberIds = new std::vector<csvDataType::ArrayType*>();
		for (int i = 0; i < m_currentNumberOfBins; i++)
		{
			csvDataType::ArrayType* init = new csvDataType::ArrayType();
			binsWithFiberIds->push_back(init);
		}

		int datasetInd = values.size();

		//check for every value inside a dataset for the corresponding bin
		for (int v = 0; v < values.size(); v++)
		{
			for (int b = 0; b < m_currentNumberOfBins; b++)
			{
				bool inside = checkRange(values.at(v), minVal + (binLength * b), minVal + (binLength * (b + 1)));

				if (!inside && b == m_currentNumberOfBins - 1)
				{
					inside = (abs(maxVal - values.at(v)) < 0.0000001);
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


		initializeMaxAmountInBins(bins, initialNumberBins);
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

	m_uniformBinningData->setBinData(binData);
	m_uniformBinningData->setBinDataObjects(binDataObjects);
	m_uniformBinningData->setMaxAmountInAllBins(m_maxAmountInAllBins);
}

bin::BinType* iACompUniformBinning::calculateBins(bin::BinType* data, int currData)
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
	double binLength = length / m_currentNumberOfBins;

	bin::BinType* bins = bin::initialize(m_currentNumberOfBins);


	for (size_t v = 0; v < amountVals; v++)
	{
		for (int b = 0; b < m_currentNumberOfBins; b++)
		{
			bool inside = checkRange(vals.at(v), min + (binLength * b), min + (binLength * (b + 1)));

			//check if the last value is the maximum value
			//otherwise this value would never be added to a bin
			if (!inside && b == m_currentNumberOfBins - 1)
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

	m_uniformBinningData->setZoomedBinData(bins);
}

int iACompUniformBinning::getMaxAmountInAllBins()
{
	return m_maxAmountInAllBins;
}


void iACompUniformBinning::initializeMaxAmountInBins(bin::BinType* bins, int initialNumberBins)
{
	for (int ind = 0; ind < initialNumberBins; ind++)
	{
		int size = bins->at(ind).size();
		if (m_maxAmountInAllBins < size)
		{
			m_maxAmountInAllBins = size;
		}
	}
}

void iACompUniformBinning::setCurrentNumberOfBins(int currentNumberOfBins)
{
	m_currentNumberOfBins = currentNumberOfBins;
}

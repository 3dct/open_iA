#include "iACompHistogramTableData.h"

//CompVis
#include "iACsvDataStorage.h"


//C++
#include <vector>

iACompHistogramTableData::iACompHistogramTableData() :
	m_maxVal(-1),
	m_minVal(-1),
	binData(new QList<bin::BinType*>()),
	zoomedBinData(bin::initialize(1)),
	binDataObjects(new QList<std::vector<csvDataType::ArrayType*>*>()),
	amountObjectsEveryDataset(new std::vector<int>),
	m_maxAmountInAllBins(0)
{
}

/************************** Setter & Getter ***************************************/

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

QList<std::vector<csvDataType::ArrayType*>*>* iACompHistogramTableData::getObjectsPerBin()
{
	return binDataObjects;
}

std::vector<int>* iACompHistogramTableData::getAmountObjectsEveryDataset()
{
	return amountObjectsEveryDataset;
}

bin::BinType* iACompHistogramTableData::getZoomedBinData()
{
	return zoomedBinData;
}

void iACompHistogramTableData::setMaxVal(double newMax)
{
	m_maxVal = newMax;
}

void iACompHistogramTableData::setMinVal(double newMin)
{
	m_minVal = newMin;
}

void iACompHistogramTableData::setBinData(QList<bin::BinType*>* newBinData)
{
	binData = newBinData;
}

void iACompHistogramTableData::setBinDataObjects(QList<std::vector<csvDataType::ArrayType*>*>* newBinDataObjects)
{
	binDataObjects = newBinDataObjects;
}

void iACompHistogramTableData::setAmountObjectsEveryDataset(std::vector<int>* newAmountObjectsEveryDataset)
{
	amountObjectsEveryDataset = newAmountObjectsEveryDataset;
}

void iACompHistogramTableData::setZoomedBinData(bin::BinType* newZoomedBinData)
{
	zoomedBinData = newZoomedBinData;
}


int iACompHistogramTableData::getMaxAmountInAllBins()
{
	return m_maxAmountInAllBins;
}
void iACompHistogramTableData::setMaxAmountInAllBins(int newMaxAmountInAllBins)
{
	m_maxAmountInAllBins = newMaxAmountInAllBins;
}

/************************** debug methods ***************************************/
void iACompHistogramTableData::debugBinDataObjects()
{
	LOG(lvlDebug, "");

	int NrOfFibers = 0;
	//DEBUG
	for (int i = 0; i < binDataObjects->size(); i++)
	{  //datasets
		LOG(lvlDebug, "Dataset " + QString::number(i));
		for (int k = 0; k < binDataObjects->at(i)->size(); k++)
		{  //bins

			csvDataType::ArrayType* data = binDataObjects->at(i)->at(k);

			for (int j = 0; j < data->size(); j++)
			{
				//LOG(lvlDebug,
				//	"fiberLabelId = " + QString::number(data->at(j).at(0)) + " --> at Bin: " + QString::number(k));
				NrOfFibers++;
			}
			LOG(lvlDebug, "number of fibers = " + QString::number(NrOfFibers));
			NrOfFibers = 0;
			LOG(lvlDebug, "");
		}
	}
	LOG(lvlDebug, "");
	LOG(lvlDebug, "#######################################################");
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
	LOG(lvlDebug," ");
	LOG(lvlDebug,"Bins: " + QString::number(amountCols));
	LOG(lvlDebug,"Bin Matrix: ");
	for (int col1 = 0; col1 < amountCols; col1++)
	{
		LOG(lvlDebug,"Bin " + QString::number(col1) + ":");
		for (int r1 = 0; r1 < ((int)input->at(col1).size()); r1++)
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

		for (int indVals = 0; indVals < ((int)curBin->size()); indVals++)
		{
			newBin->at(indVals) = (curBin->at(indVals));
		}

		output->append(newBin);
	}

	return output;
}

bin::BinType* bin::copyCells(bin::BinType* input, std::vector<vtkIdType>* indexOfCellsToCopy)
{
	bin::BinType* output = new bin::BinType();

	for(int i = 0; i < ((int)indexOfCellsToCopy->size()); i++)
	{
		output->push_back( input->at(indexOfCellsToCopy->at(i)) );
	}

	return output;
}
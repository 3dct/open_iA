#include "iACompHistogramTableData.h"


//CompVis
#include "iACsvDataStorage.h"

#include <vector>

iACompHistogramTableData::iACompHistogramTableData(iAMultidimensionalScaling* mds) : 
	m_mds(mds), 
	m_bins(10)
{
	std::vector<double>* histbinlist = csvDataType::arrayTypeToVector(m_mds->getResultMatrix());
	auto result = std::minmax_element(histbinlist->begin(), histbinlist->end());

	m_maxVal = *result.second;
	m_minVal = *result.first;

	datasets = new csvDataType::ArrayType();
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

	//DEBUG
	//csvDataType::debugArrayType(datasets);

	calculateBins();
}

void iACompHistogramTableData::calculateBins()
{
	binData = new QList<bin::BinType*>;

	//DEBUG_LOG("m_maxVal: " + QString::number(m_maxVal));
	//DEBUG_LOG("m_minVal: " + QString::number(m_minVal));

	double length = m_maxVal - m_minVal;
	double binLength = length / m_bins;

	//DEBUG_LOG("length: " + QString::number(length));
	//DEBUG_LOG("binLength: " + QString::number(binLength));

	for (int i = 0; i < amountObjectsEveryDataset->size(); i++)
	{
		std::vector<double> values = datasets->at(i);
		bin::BinType* bins = bin::initialize(m_bins);
		
		//DEBUG_LOG("");
		//DEBUG_LOG("amount values: " + QString::number(values.size())); 
		//check for every value inside a dataset for the corresponding bin
		for (int v = 0; v < values.size(); v++)
		{
			//DEBUG_LOG("");
			//DEBUG_LOG("values.at(v): " + QString::number(values.at(v)));

			for (int b = 0; b < m_bins; b++)
			{
				if (v == 0)
				{
					//	DEBUG_LOG("Bin: " + QString::number(b));
					//DEBUG_LOG("low: " + QString::number(m_minVal + (binLength * b)));
					//DEBUG_LOG("high: " + QString::number(m_minVal + (binLength * (b + 1))));
					//DEBUG_LOG("(value >= low) && (value <= high): " + QString::number(checkRange(values.at(v), m_minVal + (binLength * b), m_minVal + (binLength * (b+1)))));

				}
				bool inside = checkRange(values.at(v), m_minVal + (binLength * b), m_minVal + (binLength * (b+1)));
				if (inside)
				{
					bins->at(b).push_back(values.at(v));
					break;
				}
			}

			//DEBUG_LOG("");
		}
		
		initializeMaxAmountInBins(bins);
		binData->push_back(bins);
		
		//DEBUG
		//bin::debugBinType(bins);
	}
}

bool iACompHistogramTableData::checkRange(double value, double low, double high)
{
	return ((value >= low) && (value < high)) || (value >= m_maxVal);
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
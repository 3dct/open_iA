#include "iACompUniformBinningData.h"


iACompUniformBinningData::iACompUniformBinningData() :
	iACompHistogramTableData(),
	m_bins(10),
	m_maxAmountInAllBins(-1)
{
};

int iACompUniformBinningData::getMaxAmountInAllBins()
{
	return m_maxAmountInAllBins;
}

void iACompUniformBinningData::setMaxAmountInAllBins(int newMaxAmountInAllBins)
{
	m_maxAmountInAllBins = newMaxAmountInAllBins;
}

int iACompUniformBinningData::getInitialNumberOfBins()
{
	return m_bins;
}

QList<std::vector<double>>* iACompUniformBinningData::getBinBoundaries()
{
	return m_binsBoundaries;
}

void iACompUniformBinningData::setBinBoundaries(QList<std::vector<double>>* binBoundaries)
{
	m_binsBoundaries = binBoundaries;
}

//QList<std::vector<double>>* iACompUniformBinningData::getNumberOfObjectsPerBinAllDatasets()
//{
//	QList<std::vector<double>>* result = new QList<std::vector<double>>();
//
//	for (int dataId = 0; dataId < binData->size(); dataId++)
//	{ //datasets
//		bin::BinType* currDataset = binData->at(dataId);
//
//		std::vector<double> bins = std::vector<double>(currDataset->size(), 0);
//		for (int binId = 0; binId < currDataset->size(); binId++)
//		{ //bins
//			std::vector<double> currBin = currDataset->at(binId);
//			bins.at(binId) = currBin.size();
//		}
//		
//		result->push_back(bins);
//	}
//	
//	return result;
//}
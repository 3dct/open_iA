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

QList<std::vector<double>>* iACompUniformBinningData::getBinRange()
{ //TODO add implementation - not important
	return nullptr;
}
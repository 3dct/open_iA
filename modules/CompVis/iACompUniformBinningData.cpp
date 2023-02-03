// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

int iACompUniformBinningData::computeSturgesRule()
{
	//compute sturges rule according to dataset with the most elements in it
	std::vector<int>* numberOfObjectsAllDatasets = this->getAmountObjectsEveryDataset();
	auto result = std::minmax_element(std::begin(*numberOfObjectsAllDatasets), std::end(*numberOfObjectsAllDatasets));  
	int n = *result.second; 

	m_bins = 1 + std::log2(n);

	return m_bins;
}

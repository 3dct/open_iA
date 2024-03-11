// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompBayesianBlocksData.h"

iACompBayesianBlocksData::iACompBayesianBlocksData() :
	iACompHistogramTableData(),
	m_numberOfObjectsPerBin(nullptr),
	m_binsBoundaries(nullptr)
{
}

void iACompBayesianBlocksData::calculateNumberOfObjectsInEachBin(
	QList<std::vector<csvDataType::ArrayType*>*>* thisBinDataObjects)
{
	auto numberOfDatasets = thisBinDataObjects->size();
	m_numberOfObjectsPerBin = bin::initialize(numberOfDatasets);

	for (int v = 0; v < numberOfDatasets; v++)
	{//check for every dataset

		auto currDataset = thisBinDataObjects->at(v);
		auto currentNumberOfBins = static_cast<int>(currDataset->size());

		std::vector<double> bins;

		for (int b = 0; b < currentNumberOfBins; b++)
		{
				bins.push_back(currDataset->at(b)->size());
		}

		m_numberOfObjectsPerBin->at(v) = bins;
	}
}

void iACompBayesianBlocksData::setBinBoundaries(QList<std::vector<double>>* binBoundaries)
{
	m_binsBoundaries = binBoundaries;
}

QList<std::vector<double>>* iACompBayesianBlocksData::getBinBoundaries()
{
	return m_binsBoundaries;
}

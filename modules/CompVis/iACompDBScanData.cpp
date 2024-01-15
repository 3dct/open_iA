// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompDBScanData.h"

iACompDBScanData::iACompDBScanData() :
	iACompHistogramTableData(),
	//m_numberOfObjectsPerBin(nullptr), // unused!
	m_binsBoundaries(nullptr)
{
}

void iACompDBScanData::calculateNumberOfObjectsInEachBin(
	QList<std::vector<csvDataType::ArrayType*>*>* )
{
	//TODO
}

void iACompDBScanData::setBinBoundaries(QList<std::vector<double>>* binBoundaries)
{
	m_binsBoundaries = binBoundaries;
}

QList<std::vector<double>>* iACompDBScanData::getBinBoundaries()
{
	return m_binsBoundaries;
}

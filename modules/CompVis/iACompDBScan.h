// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompBinning.h"
#include "DBScan/dbscan.hpp"

class iACompDBScanData;

class iACompDBScan : public iACompBinning
{
public:

	iACompDBScan(
		iACsvDataStorage* dataStorage, bin::BinType* datasets);

	//calculate the binning for the data points
	virtual void calculateBins();

	//calculates the bin datastructure for (a) specifically selected bin(s)
	virtual bin::BinType* calculateBins(bin::BinType* data, int currData);

	virtual void setDataStructure(iACompHistogramTableData* datastructure);

private:

	iACompDBScanData* m_dbData;
};

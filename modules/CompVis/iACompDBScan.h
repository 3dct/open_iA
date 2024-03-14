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
	void calculateBins() override;

	//calculates the bin datastructure for (a) specifically selected bin(s)
	bin::BinType* calculateBins(bin::BinType* data, int currData) override;

	void setDataStructure(iACompHistogramTableData* datastructure) override;

private:

	iACompDBScanData* m_dbData;
};

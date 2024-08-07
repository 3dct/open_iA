// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompHistogramTableData.h"

class iACompUniformBinningData : public iACompHistogramTableData
{
public:

	iACompUniformBinningData();

	//returns for every dataset each bin defined by its lower boundary
	virtual QList<std::vector<double>>* getBinBoundaries() override;
	//set for each dataset for each bin its lowerBoundary
	virtual void setBinBoundaries(QList<std::vector<double>>* binBoundaries) override;

	//returns the maximum amount of numbers in all bins --> i.e. there are maximum 5 values in one bin
	int getMaxAmountInAllBins();
	void setMaxAmountInAllBins(int newMaxAmountInAllBins);

	int getInitialNumberOfBins();

	//compute the initial number of bins
	int computeSturgesRule();

private:

	//amount of bins in the histogram for all rows/datasets
	int m_bins;

	//maximum amount of numbers in a bin (calculated for all bins)
	int m_maxAmountInAllBins;

};

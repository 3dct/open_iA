// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"

#include <memory>

class iAXRFData;

class iASpectraHistograms
{
public:
	iASpectraHistograms(std::shared_ptr<iAXRFData> xrfData, long numBins = 1, double minCount = 0, double maxCount = 0);
	~iASpectraHistograms();
	void compute(long numBins, double maxCount, double minCount);
	CountType * histData() const;
	long numBins() const;
	size_t numHist() const;
	CountType maxValue() const;
private:
	void computeHistograms();
	void computeMaximumVal();

private:
	long	m_numBins;			///< number of bins in a histogram
	size_t	m_numHistograms;	///< number of energy bins is the number of histograms
	double	m_countRange[2];	///< range of XRF

	double			m_binWidth;			///< width of a histogram bin
	CountType	*	m_histData;			///< raw data containing a 2D array, first dimension - histograms, second - bins

	std::shared_ptr<iAXRFData> m_xrfData;	///< pointer to the input xrf data set
	CountType	m_maxValue;			///< maximum value of all histograms' bins
};

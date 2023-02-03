// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAEnergySpectrum.h"
#include "iASpectrumFunction.h"

#include <iAPlotData.h>

#include <QSharedPointer>

#include <vector>

class iAXRFData;
class iASpectraHistograms;

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot<size_t, unsigned int> FunctionalBoxPlot;

// TODO: merge with iAHistogramData?
// ideally: split up into 3 different usages of iAHistogramData
class iAAccumulatedXRFData: public iAPlotData
{
public:
	enum AccumulateFct
	{
		fctMax = 0,
		fctAvg = 1,
		fctMin = 2,

		fctDefault = fctMax,
	};
	iAAccumulatedXRFData(QSharedPointer<iAXRFData> data, double minEnergy, double maxEnergy);
	// { remove if merged with iAHistogramData:
	double const * xBounds() const override;
	DataType const * yBounds() const override;
	DataType yValue(size_t idx) const override;
	double xValue(size_t idx) const override;
	size_t valueCount() const override;
	size_t nearestIdx(DataType dataX) const override;
	QString toolTipText(DataType dataX) const override;
	double spacing() const;
	// }

	void setFct(int fctIdx);
	void retrieveHistData(long numBin_in, DataType * &data_out, size_t &numHist_out, DataType &maxValue_out);
	CountType spectraHistogramMax() const;
	DataType const * avgData() const;
	FunctionalBoxPlot* functionalBoxPlot();
private:
	void computeSpectraHistograms(long numBins);
	iAAccumulatedXRFData(iAAccumulatedXRFData const & other);
	iAAccumulatedXRFData operator=(iAAccumulatedXRFData const & other);
	void calculateStatistics();
	void calculateFunctionBoxplots();
	void createSpectrumFunctions();
	std::vector<iAFunction<size_t, unsigned int> *> const & spectrumFunctions();

	QSharedPointer<iAXRFData> m_xrfData;
	CountType* m_minimum;
	CountType* m_maximum;
	CountType* m_average;
	AccumulateFct m_accumulateFct;
	double m_xBounds[2];
	DataType m_yBounds[2];
	FunctionalBoxPlot* m_functionalBoxplotData;
	std::vector<iAFunction<size_t, unsigned int> *> m_spectrumFunctions;
	QSharedPointer<iASpectraHistograms>	m_spectraHistograms;
};

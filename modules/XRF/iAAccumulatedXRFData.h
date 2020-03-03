/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAEnergySpectrum.h"
#include "iASpectrumFunction.h"

#include <charts/iAPlotData.h>

#include <QSharedPointer>

#include <vector>

class iAXRFData;
class iASpectraHistograms;

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot<size_t, unsigned int> FunctionalBoxPlot;

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
	double spacing() const override;
	double const * xBounds() const override;
	DataType const * yBounds() const override;
	DataType const * rawData() const override;
	size_t numBin() const override;
	void setFct(int fctIdx);
	void retrieveHistData(long numBin_in, DataType * &data_out, size_t &numHist_out, DataType &maxValue_out);
	CountType spectraHistogramMax() const;
	DataType const * avgData() const;
	FunctionalBoxPlot* functionalBoxPlot();
private:
	void computeSpectraHistograms( long numBins );
	iAAccumulatedXRFData(iAAccumulatedXRFData const & other);
	iAAccumulatedXRFData operator=(iAAccumulatedXRFData const & other);
	void calculateStatistics();
	void calculateFunctionBoxplots();
	void createSpectrumFunctions();
	std::vector<iAFunction<size_t, unsigned int> *> const & spectrumFunctions();

	QSharedPointer<iAXRFData> m_xrfData;
	CountType* m_maximum;
	CountType* m_minimum;
	CountType* m_average;
	AccumulateFct m_accumulateFct;
	double m_xBounds[2];
	DataType m_yBounds[2];
	FunctionalBoxPlot* m_functionalBoxplotData;
	std::vector<iAFunction<size_t, unsigned int> *> m_spectrumFunctions;
	QSharedPointer<iASpectraHistograms>	m_spectraHistograms;
};

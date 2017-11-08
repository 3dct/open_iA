/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAAbstractDiagramData.h"
#include "iAEnergySpectrum.h"
#include "iASpectrumFunction.h"

#include <vector>

#include <QSharedPointer>

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
	double GetSpacing() const override;
	double const * XBounds() const override;
	DataType const * YBounds() const override;
	DataType const * GetRawData() const override;
	size_t GetNumBin() const override;
	void SetFct(int fctIdx);
	void RetrieveHistData(long numBin_in, DataType * &data_out, size_t &numHist_out, DataType &maxValue_out);
	CountType GetSpectraHistogramMax() const;
	DataType const * GetAvgData() const;
	FunctionalBoxPlot* const GetFunctionalBoxPlot();
private:
	void ComputeSpectraHistograms( long numBins );
	iAAccumulatedXRFData(iAAccumulatedXRFData const & other);
	iAAccumulatedXRFData operator=(iAAccumulatedXRFData const & other);
	void calculateStatistics();
	void calculateFunctionBoxplots();
	void createSpectrumFunctions();
	std::vector<iAFunction<size_t, unsigned int> *> const & GetSpectrumFunctions();

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

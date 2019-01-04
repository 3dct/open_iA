/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAAccumulatedXRFData.h"

#include "iASpectraHistograms.h"
#include "iAXRFData.h"

#include <iAFunctionalBoxplot.h>
#include <iATypedCallHelper.h>

#include <vtkImageData.h>
#include <vtkImageResample.h>

#include <cassert>
#include <limits>

iAAccumulatedXRFData::iAAccumulatedXRFData(QSharedPointer<iAXRFData> data, double minEnergy, double maxEnergy) :
	m_xrfData(data),
	m_spectraHistograms(new iASpectraHistograms(data)),
	m_minimum(new CountType[m_xrfData->size()]),
	m_maximum(new CountType[m_xrfData->size()]),
	m_average(new CountType[m_xrfData->size()]),
	m_functionalBoxplotData(0)
{
	m_xBounds[0] = minEnergy;
	m_xBounds[1] = maxEnergy;
	m_yBounds[1] = 0;
	m_yBounds[0] = std::numeric_limits<double>::max();
	calculateStatistics();
	SetFct(fctDefault);
}

double iAAccumulatedXRFData::GetSpacing() const
{
	return (m_xBounds[1] - m_xBounds[0]) / GetNumBin();
}

double const * iAAccumulatedXRFData::XBounds() const
{
	return m_xBounds;
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::GetRawData() const
{
	switch (m_accumulateFct)
	{
	case fctAvg:
		return m_average;
	case fctMin:
		return m_minimum;
	default:
	case fctMax:
		return m_maximum;
	}
}

size_t iAAccumulatedXRFData::GetNumBin() const
{
	return m_xrfData->size();
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::YBounds() const
{
	return m_yBounds;
}

CountType iAAccumulatedXRFData::GetSpectraHistogramMax() const
{
	return m_spectraHistograms->maxValue();
}

void iAAccumulatedXRFData::SetFct(int fctIdx)
{
	m_accumulateFct = static_cast<AccumulateFct>(fctIdx);
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::GetAvgData() const
{
	return m_average;
}

void iAAccumulatedXRFData::ComputeSpectraHistograms( long numBins )
{
	m_spectraHistograms->compute(numBins, m_yBounds[1], m_yBounds[0]);
}

void iAAccumulatedXRFData::RetrieveHistData( long numBin_in, DataType * &data_out, size_t &numHist_out, DataType &maxValue_out )
{
	ComputeSpectraHistograms(numBin_in);
	data_out = m_spectraHistograms->histData();
	numHist_out = m_spectraHistograms->numHist();
	maxValue_out = m_spectraHistograms->maxValue();
}

namespace
{
	iASpectrumFunction * createSpectrumFunction(QSharedPointer<iAXRFData const> xrfData, int x, int y, int z)
	{
		iASpectrumFunction *result = new iASpectrumFunction();
		for (size_t i=0; i<xrfData->size(); ++i)
		{
			result->insert(std::make_pair(i, static_cast<unsigned int>(xrfData->GetImage(i)->GetScalarComponentAsFloat(x, y, z, 0))));
		}
		return result;
	}

	template <typename T>
	void calculateLevelStats(void* dataVoidPtr, int count, double &avg, double &max, double &min)
	{
		T* data = static_cast<T*>(dataVoidPtr);
		double sum = 0;
		unsigned long relevantCount = 0;
		for (int i=0; i<count; ++i)
		{
			sum += data[i];
			if (data[i] != 0)
			{
				relevantCount++;
			}
			if (data[i] > max)
			{
				max = data[i];
			}
			if (data[i] < min)
			{
				min = data[i];
			}
		}
		avg = sum / relevantCount;
	}
}


void iAAccumulatedXRFData::calculateStatistics()
{
	iAXRFData::Iterator it = m_xrfData->begin();
	if (it == m_xrfData->end())
	{
		return;
	}
	size_t i=0;
	int extent[6];
	(*it)->GetExtent(extent);
	int xrange = extent[1]-extent[0]+1;
	int yrange = extent[3]-extent[2]+1;
	int zrange = extent[5]-extent[4]+1;
	unsigned int count = xrange*yrange*zrange;
	while (it != m_xrfData->end())
	{
		vtkSmartPointer<vtkImageData> img1 = *it;
		// just checks: begin
		assert (img1->GetNumberOfScalarComponents() == 1);
		int extent[6];
		img1->GetExtent(extent);
		double * range = img1->GetScalarRange();
		assert( ((extent[1]-extent[0]+1) * (extent[3]-extent[2]+1) * (extent[5]-extent[4]+1)) == count );
		// end checks
		int type = img1->GetScalarType();
		double avg = 0.0;
		double max = 0.0;
		double min = std::numeric_limits<double>::max();
		VTK_TYPED_CALL(calculateLevelStats, type, img1->GetScalarPointer(), count, avg, max, min);
		m_average[i] = avg;
		m_maximum[i] = max;
		m_minimum[i] = min;
		if(max > m_yBounds[1])
			m_yBounds[1] = max;
		if(min < m_yBounds[0])
			m_yBounds[0] = min;
		++it;
		++i;
	}

	//workaround if XRF values are negative (in our case due to the FDK reco nature)
	if(m_yBounds[0] < 0.0 )
		for(int j=0; j<i; ++j)
		{
			m_average[j] -= m_yBounds[0];
			m_maximum[j] -= m_yBounds[0];
			m_minimum[j] -= m_yBounds[0];
		}
}

void iAAccumulatedXRFData::createSpectrumFunctions()
{
	vtkSmartPointer<vtkImageData> img = m_xrfData->GetImage(0);
	int extent[6];
	img->GetExtent(extent);
	for (int x = extent[0]; x <= extent[1]; ++x)
	{
		for (int y = extent[2]; y <= extent[3]; ++y)
		{
			for (int z = extent[4]; z <= extent[5]; ++z)
			{
				iASpectrumFunction *wrap = createSpectrumFunction(m_xrfData, x, y, z);
				m_spectrumFunctions.push_back(wrap);
			}
		}
	}
	size_t numSpectra = (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1);
}

std::vector<iAFunction<size_t, unsigned int> *> const & iAAccumulatedXRFData::GetSpectrumFunctions()
{
	if (m_spectrumFunctions.size() == 0)
	{
		createSpectrumFunctions();
	}
	return m_spectrumFunctions;
}

void iAAccumulatedXRFData::calculateFunctionBoxplots()
{
	assert(!m_functionalBoxplotData);
	ModifiedDepthMeasure<size_t, unsigned int> measure;
	std::vector<iAFunction<size_t, unsigned int> *> functions = GetSpectrumFunctions();
	m_functionalBoxplotData = new iAFunctionalBoxplot<size_t, unsigned int>(
		functions, &measure, 2);
}

FunctionalBoxPlot* const iAAccumulatedXRFData::GetFunctionalBoxPlot()
{
	if (!m_functionalBoxplotData)
	{
		calculateFunctionBoxplots();
	}
	return m_functionalBoxplotData;
}

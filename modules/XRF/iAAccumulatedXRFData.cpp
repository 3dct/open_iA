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
 
#include "pch.h"
#include "iAAccumulatedXRFData.h"

#include "iAFunctionalBoxplot.h"
#include "iASpectraHistograms.h"
#include "iAXRFData.h"

#include <vtkImageData.h>
#include <vtkImageResample.h>

#include <cassert>
#include <limits>

iAAccumulatedXRFData::iAAccumulatedXRFData(QSharedPointer<iAXRFData> data, double minEnergy, double maxEnergy):
	m_xrfData(data),
	m_spectraHistograms(new iASpectraHistograms(data)),
	m_minimum(new CountType[m_xrfData->size()]),
	m_maximum(new CountType[m_xrfData->size()]),
	m_average(new CountType[m_xrfData->size()]),
	m_totalMaximum(0),
	m_totalMinimum(std::numeric_limits<double>::max()),
	m_functionalBoxplotData(0)
{
	calculateStatistics();
	SetFct(fctDefault);
	dataRange[0] = minEnergy;
	dataRange[1] = maxEnergy;
}

double iAAccumulatedXRFData::GetSpacing() const
{
	return (dataRange[1] - dataRange[0]) / GetNumBin();
}

double const * iAAccumulatedXRFData::XBounds() const
{
	return dataRange;
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::GetData() const
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

iAAccumulatedXRFData::DataType iAAccumulatedXRFData::GetMaxValue() const
{
	return m_totalMaximum;
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
	double maxCount = m_totalMaximum;
	double minCount = m_totalMinimum;
	m_spectraHistograms->compute(numBins, maxCount, minCount);
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
		iASpectrumFunction *result = new iASpectrumFunction(xrfData->size());
		for (size_t i=0; i<xrfData->size(); ++i)
		{
			result->set(i, static_cast<unsigned int>(xrfData->GetImage(i)->GetScalarComponentAsFloat(x, y, z, 0)));
		}
		return result;
	}

	template <typename T>
	void calculateLevelStats(T* data, int count, double &avg, double &max, double &min)
	{
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
		switch (type)
		{
		case VTK_CHAR:
			calculateLevelStats<char>(static_cast<char*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_SIGNED_CHAR:
			calculateLevelStats<char>(static_cast<char*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_UNSIGNED_CHAR:
			calculateLevelStats<unsigned char>(static_cast<unsigned char*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_SHORT:
			calculateLevelStats<short>(static_cast<short*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_UNSIGNED_SHORT:
			calculateLevelStats<unsigned short>(static_cast<unsigned short*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_INT:
			calculateLevelStats<int>(static_cast<int*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_UNSIGNED_INT:
			calculateLevelStats<unsigned int>(static_cast<unsigned int*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_LONG:
			calculateLevelStats<long>(static_cast<long*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_UNSIGNED_LONG:
			calculateLevelStats<unsigned long>(static_cast<unsigned long*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_FLOAT:
			calculateLevelStats<float>(static_cast<float*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		case VTK_DOUBLE:
			calculateLevelStats<double>(static_cast<double*>(img1->GetScalarPointer()), count, avg, max, min);
			break;
		default:
			avg = 0.0;
			max = 0.0;
			// TODO: LOG ERROR!
			break;
		}
		m_average[i] = avg;
		m_maximum[i] = max;
		m_minimum[i] = min;
		if(max > m_totalMaximum)
			m_totalMaximum = max;
		if(min < m_totalMinimum)
			m_totalMinimum = min;
		++it;
		++i;
	}

	//workaround if XRF values are negative (in our case due to the FDK reco nature)
	if( m_totalMinimum < 0.0 )
		for(int j=0; j<i; ++j)
		{
			m_average[j] -= m_totalMinimum;
			m_maximum[j] -= m_totalMinimum;
			m_minimum[j] -= m_totalMinimum;
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
		functions, /*argMin=*/0, /*argMax=*/m_xrfData->size()-1,
		&measure, 2);
}

FunctionalBoxPlot* const iAAccumulatedXRFData::GetFunctionalBoxPlot()
{
	if (!m_functionalBoxplotData)
	{
		calculateFunctionBoxplots();
	}
	return m_functionalBoxplotData;
}

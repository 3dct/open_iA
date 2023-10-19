// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAccumulatedXRFData.h"

#include "iASpectraHistograms.h"
#include "iAXRFData.h"

#include <iAFunctionalBoxplot.h>
#include <iAMathUtility.h>
#include <iATypedCallHelper.h>

#include <vtkImageData.h>
#include <vtkImageResample.h>

#include <cassert>
#include <limits>

iAAccumulatedXRFData::iAAccumulatedXRFData(std::shared_ptr<iAXRFData> data, double minEnergy, double maxEnergy) :
	iAPlotData("Accumulated Spectrum", iAValueType::Continuous),
	m_xrfData(data),
	m_minimum(new CountType[m_xrfData->size()]),
	m_maximum(new CountType[m_xrfData->size()]),
	m_average(new CountType[m_xrfData->size()]),
	m_functionalBoxplotData(nullptr),
	m_spectraHistograms(new iASpectraHistograms(data))
{
	m_xBounds[0] = minEnergy;
	m_xBounds[1] = maxEnergy;
	m_yBounds[1] = 0;
	m_yBounds[0] = std::numeric_limits<double>::max();
	calculateStatistics();
	setFct(fctDefault);
}

double iAAccumulatedXRFData::spacing() const
{
	return (m_xBounds[1] - m_xBounds[0]) / valueCount();
}

double const * iAAccumulatedXRFData::xBounds() const
{
	return m_xBounds;
}

iAAccumulatedXRFData::DataType iAAccumulatedXRFData::yValue(size_t idx) const
{
	switch (m_accumulateFct)
	{
	case fctAvg:
		return m_average[idx];
	case fctMin:
		return m_minimum[idx];
	default:
	case fctMax:
		return m_maximum[idx];
	}
}

double iAAccumulatedXRFData::xValue(size_t idx) const
{
	return xBounds()[0] + spacing() * idx;
}

size_t iAAccumulatedXRFData::valueCount() const
{
	return m_xrfData->size();
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::yBounds() const
{
	return m_yBounds;
}

size_t iAAccumulatedXRFData::nearestIdx(double dataX) const
{
	double binRng[2] = {0, static_cast<double>(valueCount())};
	return clamp(static_cast<size_t>(0), valueCount() - 1, static_cast<size_t>(mapValue(xBounds(), binRng, dataX)));
}

QString iAAccumulatedXRFData::toolTipText(iAPlotData::DataType dataX) const
{
	size_t idx = nearestIdx(dataX);
	auto bStart = xValue(idx);
	auto bEnd = xValue(idx + 1);
	if (valueType() == iAValueType::Discrete || valueType() == iAValueType::Categorical)
	{
		bStart = static_cast<int>(bStart);
		bEnd = static_cast<int>(bEnd - 1);
	}
	auto freq = yValue(idx);
	return QString("%1-%2: %3").arg(bStart).arg(bEnd).arg(freq);
}

CountType iAAccumulatedXRFData::spectraHistogramMax() const
{
	return m_spectraHistograms->maxValue();
}

void iAAccumulatedXRFData::setFct(int fctIdx)
{
	m_accumulateFct = static_cast<AccumulateFct>(fctIdx);
}

iAAccumulatedXRFData::DataType const * iAAccumulatedXRFData::avgData() const
{
	return m_average;
}

void iAAccumulatedXRFData::computeSpectraHistograms( long numBins )
{
	m_spectraHistograms->compute(numBins, m_yBounds[1], m_yBounds[0]);
}

void iAAccumulatedXRFData::retrieveHistData( long numBin_in, DataType * &data_out, size_t &numHist_out, DataType &maxValue_out )
{
	computeSpectraHistograms(numBin_in);
	data_out = m_spectraHistograms->histData();
	numHist_out = m_spectraHistograms->numHist();
	maxValue_out = m_spectraHistograms->maxValue();
}

namespace
{
	iASpectrumFunction * createSpectrumFunction(std::shared_ptr<iAXRFData const> xrfData, int x, int y, int z)
	{
		iASpectrumFunction *result = new iASpectrumFunction();
		for (size_t i=0; i<xrfData->size(); ++i)
		{
			result->insert(std::make_pair(i, static_cast<unsigned int>(xrfData->image(i)->GetScalarComponentAsFloat(x, y, z, 0))));
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
		int extentImg1[6];
		img1->GetExtent(extentImg1);
		//double * range = img1->GetScalarRange();
		assert( (static_cast<unsigned>(extentImg1[1]- extentImg1[0]+1) * (extentImg1[3]- extentImg1[2]+1) * (extentImg1[5]- extentImg1[4]+1)) == count );
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
	if (m_yBounds[0] < 0.0)
	{
		for (size_t j = 0; j < i; ++j)
		{
			m_average[j] -= m_yBounds[0];
			m_maximum[j] -= m_yBounds[0];
			m_minimum[j] -= m_yBounds[0];
		}
	}
}

void iAAccumulatedXRFData::createSpectrumFunctions()
{
	vtkSmartPointer<vtkImageData> img = m_xrfData->image(0);
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
	//size_t numSpectra = (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1);
}

std::vector<iAFunction<size_t, unsigned int> *> const & iAAccumulatedXRFData::spectrumFunctions()
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
	iAModifiedDepthMeasure<size_t, unsigned int> measure;
	std::vector<iAFunction<size_t, unsigned int> *> functions = spectrumFunctions();
	m_functionalBoxplotData = new iAFunctionalBoxplot<size_t, unsigned int>(
		functions, &measure, 2);
}

FunctionalBoxPlot* iAAccumulatedXRFData::functionalBoxPlot()
{
	if (!m_functionalBoxplotData)
	{
		calculateFunctionBoxplots();
	}
	return m_functionalBoxplotData;
}

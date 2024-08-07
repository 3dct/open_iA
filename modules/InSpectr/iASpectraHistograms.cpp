// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASpectraHistograms.h"

#include "iAXRFData.h"

#include <iATypedCallHelper.h>

#include <vtkImageData.h>

#include <cassert>
#include <cmath>

iASpectraHistograms::iASpectraHistograms(std::shared_ptr<iAXRFData> xrfData, long numBins, double minCount, double maxCount ) :
	m_numBins(numBins),
	m_histData(nullptr),
	m_xrfData(xrfData),
	m_maxValue(0)
{
	m_countRange[0] = minCount; m_countRange[1] = maxCount;
	m_numHistograms = m_xrfData->size();
	size_t count = m_numHistograms*m_numBins;
	m_histData = new CountType[count];
	std::fill_n(m_histData, count, CountTypeNull);
	m_binWidth = (m_countRange[1] - m_countRange[0]) / m_numBins;
}

iASpectraHistograms::~iASpectraHistograms()
{
	delete [] m_histData;
	m_histData = nullptr;
}

template <typename T>
void computeHistogram(void* scalarPtr, long & count, double & binWidth, CountType * histData_out, double * range)
{
	T* data = static_cast<T*>(scalarPtr);
	for (long i=0; i<count; ++i)
	{
		double bin = ( (double)data[i] - range[0] ) / binWidth;
		if ( bin > 0.0 && std::fmod(bin, 1.0) == 0.0 )
			--bin;
		size_t binInd = bin;
		histData_out[binInd]++;
	}
}

void iASpectraHistograms::computeHistograms( )
{
	iAXRFData::Iterator it = m_xrfData->begin();
	if (it == m_xrfData->end())
		return;

	std::fill_n(m_histData, m_numHistograms*m_numBins, CountTypeNull);

	int extent[6];
	(*it)->GetExtent(extent);
	int xrange = extent[1]-extent[0]+1;
	int yrange = extent[3]-extent[2]+1;
	int zrange = extent[5]-extent[4]+1;
	long count = xrange*yrange*zrange;
	CountType * curHistPtr = m_histData;
	while (it != m_xrfData->end())
	{
		vtkSmartPointer<vtkImageData> curImg = *it;
		// just checks: begin
		assert (curImg->GetNumberOfScalarComponents() == 1);
		int curImgExtent[6];
		curImg->GetExtent(curImgExtent);
		assert( ((curImgExtent[1]- curImgExtent[0]+1) * (curImgExtent[3]- curImgExtent[2]+1) * (curImgExtent[5]- curImgExtent[4]+1)) == count );
		// end checks
		int type = curImg->GetScalarType();
		VTK_TYPED_CALL(computeHistogram, type, curImg->GetScalarPointer(), count, m_binWidth, curHistPtr, m_countRange);
		curHistPtr += m_numBins;
		++it;
	}
	computeMaximumVal();
}

void iASpectraHistograms::computeMaximumVal()
{
	m_maxValue = 0;
	size_t count = m_numHistograms*m_numBins;
	for (size_t i = 0; i < count; ++i)
	{
		if(m_histData[i] > m_maxValue)
			m_maxValue = m_histData[i];
	}
}

void iASpectraHistograms::compute( long numBins, double maxCount, double minCount )
{
	bool doReallocate = false;
	bool doRecompute = false;
	if(m_numBins != numBins)
	{
		m_numBins = numBins;
		doReallocate = doRecompute = true;
	}
	if(m_countRange[0] != minCount || m_countRange[1] != maxCount)
	{
		m_countRange[0] = minCount; m_countRange[1] = maxCount;
		doRecompute = true;
	}

	if(doReallocate)
	{
		m_numHistograms = m_xrfData->size();
		delete [] m_histData;
		m_histData = new CountType[m_numHistograms*m_numBins];
	}

	if(doRecompute)
	{
		m_binWidth = (m_countRange[1] - m_countRange[0]) / m_numBins;
		computeHistograms();
	}
}

CountType * iASpectraHistograms::histData() const
{
	return m_histData;
}

long iASpectraHistograms::numBins() const
{
	return m_numBins;
}

size_t iASpectraHistograms::numHist() const
{
	return m_numHistograms;
}

CountType iASpectraHistograms::maxValue() const
{
	return m_maxValue;
}

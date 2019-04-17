/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASpectraHistograms.h"

#include "iAXRFData.h"

#include <iATypedCallHelper.h>

#include <vtkImageData.h>

#include <cassert>

iASpectraHistograms::iASpectraHistograms(QSharedPointer<iAXRFData> xrfData, long numBins, double minCount, double maxCount ) :
		m_xrfData(xrfData),
		m_numBins(numBins),
		m_histData(0),
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
	m_histData = 0;
}

template <typename T>
void computeHistogram(void* scalarPtr, long & count, double & binWidth, CountType * histData_out, double * range)
{
	T* data = static_cast<T*>(scalarPtr);
	for (long i=0; i<count; ++i)
	{
		double bin = ( (double)data[i] - range[0] ) / binWidth;
		if ( bin > 0.0 && fmod(bin, 1.0) == 0.0 )
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

	size_t i = 0;
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
		int extent[6];
		curImg->GetExtent(extent);
		assert( ((extent[1]-extent[0]+1) * (extent[3]-extent[2]+1) * (extent[5]-extent[4]+1)) == count );
		// end checks
		int type = curImg->GetScalarType();
		VTK_TYPED_CALL(computeHistogram, type, curImg->GetScalarPointer(), count, m_binWidth, curHistPtr, m_countRange);
		curHistPtr += m_numBins;
		++it;
		++i;	
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
		delete [] m_histData; m_histData = 0;
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

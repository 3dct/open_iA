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

#include "iAMaximumDistanceFilter.h"

#include <iAConsole.h>

template <class TImageType>
iAMaximumDistanceFilter<TImageType>::iAMaximumDistanceFilter()
{
	m_BTIFFilter = BTIFType::New();
	m_Threshold = 0;
	m_high_intensity = 0;
	m_low_intensity = 0;
	m_binWidth = 0;
	m_centre = 0;
}

template <class TImageType>
void iAMaximumDistanceFilter<TImageType>::GenerateData()
{
	if (std::numeric_limits<typename TImageType::PixelType>::lowest() < 0)
	{
		DEBUG_LOG("Signed pixel type not supported by maximum distance filter!");
		return;
	}
	if (!std::is_integral<typename TImageType::PixelType>::value)
	{
		DEBUG_LOG("Non-integral pixel type not supported by maximum distance filter!");
		return;
	}
	if (m_centre == 0)
	{
		m_centre = std::numeric_limits<typename TImageType::PixelType>::max() / (m_binWidth * 2);
	}
	assert(m_binWidth != 0);
	m_inImage = this->GetInput();
	ConstIteratorType in_Iter (m_inImage, m_inImage->GetLargestPossibleRegion());

	// histogram:
	int noofBins = (std::numeric_limits<typename TImageType::PixelType>::max() / m_binWidth) + 1;
	std::vector<long long> histogram (noofBins, 0 );
	for ( in_Iter.GoToBegin(); !in_Iter.IsAtEnd(); ++in_Iter )
	{
		PixelType binIndex = static_cast<PixelType> (in_Iter.Get()/ m_binWidth);
		histogram[binIndex] += 1;
	}

	// TODO: Proper peak determination!

	int start = 0, end = 0;
	//remove the first intensity
	for(int i = 0; i < noofBins; ++i )
	{
		if (histogram[i] != 0 )
		{
			start = i;
			break;
		}
	}

	//remove the last intensity
	for(int i = (noofBins-1); i >= 0; --i )
	{
		if (histogram[i] != 0 )
		{
			end = i;
			break;
		}
	}

	//find the high intensity peak
	long long high_freq = 0;
	for(int i = (start+1); i < (end-1); ++i )
	{
		if (histogram[i] > high_freq )
		{
			high_freq = histogram[i];
			m_high_intensity = i;
		}
	}

	//find the low intensity peak
	long long low_freq = 0;
	for( int i = (start+1); i < m_centre && i < m_high_intensity; ++i )
	{
		if (histogram[i] > low_freq )
		{
			low_freq = histogram[i];
			m_low_intensity = i;
		}
	}

	double max_diff = 0;
	//calculation of angle CDE
	double alphaWithoutATan = static_cast<double>(histogram[m_high_intensity] - histogram[m_low_intensity]) / (m_high_intensity - m_low_intensity);
	double alpha = atan( alphaWithoutATan );
	int intensity = 0;
	for(int i = m_low_intensity; i < m_high_intensity; ++i )
	{
		//calculation of new height                // first atan and then tan -> one is inverse of the other, so couldn't we simply skip both?
		double tanatanalpha = tan(alpha);
		double new_height =  ((static_cast<double>(i) - m_low_intensity) * tanatanalpha) + histogram[m_low_intensity];

		// the difference between the new height and the original histogram height
		double diff = new_height - histogram[i];

		//check whether the difference is greater than the previous difference
		//if YES change the max_difference value and the threshold intensity value
		//if NO check for the next intensity
		if ( max_diff < diff )
		{
			max_diff = diff;
			intensity = i;
		}
	}
	// 0.5 is an approximation factor
	m_Threshold = (intensity + 0.5) * m_binWidth;

	m_BTIFFilter->SetLowerThreshold( 0 );
	m_BTIFFilter->SetUpperThreshold(m_Threshold);
	m_BTIFFilter->SetOutsideValue( 0 );
	m_BTIFFilter->SetInsideValue(std::numeric_limits<typename TImageType::PixelType>::max() - 1);
	m_BTIFFilter->SetInput(this->GetInput());

	m_BTIFFilter->GraftOutput( this->GetOutput() );
	m_BTIFFilter->Update();
	this->GraftOutput( m_BTIFFilter->GetOutput() );
}

template<class TInputImage>
void iAMaximumDistanceFilter<TInputImage>::SetCentre(double lowIntensity)
{
	m_centre = lowIntensity / m_binWidth;
}

template<class TInputImage>
void iAMaximumDistanceFilter<TInputImage>::SetBinWidth(double binWidth)
{
	m_binWidth = binWidth;
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetOutThreshold()
{
	return static_cast<int>(m_Threshold);
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetLowIntensity()
{
	return static_cast<int>((m_low_intensity + 0.5) * m_binWidth);
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetHighIntensity()
{
	return static_cast<int>((m_high_intensity + 0.5) * m_binWidth);
}

template <class TImageType>
void iAMaximumDistanceFilter<TImageType>::PrintSelf(std::ostream& os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "Threshold:" << m_Threshold
	   << std::endl;
}

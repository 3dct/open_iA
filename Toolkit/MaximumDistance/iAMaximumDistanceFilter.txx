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
#pragma once

#include "iAMaximumDistanceFilter.h"

// Traits type to prevent overflow in Threshold below
template<typename T>
struct MyThreshType{
	static const int Threshold = 65534;
};

template<>
struct MyThreshType<char>{
	static const int Threshold = 127;
};

template<>
struct MyThreshType<unsigned char>{
	static const int Threshold = 254;
};

template <class TImageType>
iAMaximumDistanceFilter<TImageType>::iAMaximumDistanceFilter()
{
	m_BTIFFilter = BTIFType::New();
	m_Threshold = 0;
	m_intensity = 0;
	m_i = 0;
	m_high_intensity = 0;
	m_low_intensity = 0;
	m_high_freq = 0;
	m_low_freq = 0;
	m_start = 0;
	m_end = 0;
	m_first_value = 0;
	m_last_value = 0;
}

template <class TImageType>
void iAMaximumDistanceFilter<TImageType>::GenerateData()
{
	m_InImage = (  this->GetInput() );
	ConstIteratorType in_Iter (m_InImage, m_InImage->GetLargestPossibleRegion());

	//get the starting pixel value in the image
	for ( in_Iter.GoToBegin(); !in_Iter.IsAtEnd(); ++in_Iter )
	{
		if ( in_Iter.Get() < m_first_value )
		{
			m_first_value = in_Iter.Get();	
		} 
	}

	//get the maximum pixel value in the image
	for ( in_Iter.GoToBegin(); !in_Iter.IsAtEnd(); ++in_Iter )
	{
		if ( in_Iter.Get() > m_last_value )
		{
			m_last_value = in_Iter.Get();	
		} 
	}


	//calculate the number of bins required
	//int m_NoofBins = ( (m_last_value - m_first_value) / m_Bins ) + 1;
	int m_NoofBins = (65536/m_Bins)+1;

	//create the container for the histogram
	std::vector<long long> m_VolumeCount ( m_NoofBins, 0 );

	//histogram code
	for ( in_Iter.GoToBegin(); !in_Iter.IsAtEnd(); ++in_Iter )
	{
		PixelType Pixel_test = static_cast<PixelType> (in_Iter.Get()/m_Bins);
		long long value = m_VolumeCount[Pixel_test];	
		m_VolumeCount[Pixel_test] = value + 1;	
	}

	//remove the first intensity
	for( m_i = 0; m_i < m_NoofBins; m_i++ )
	{
		if ( m_VolumeCount[m_i] != 0 )
		{
			m_start = m_i;
			break;
		}
	}

	//remove the last intensity
	for( m_i = (m_NoofBins-1); m_i >= 0; m_i-- )
	{
		if ( m_VolumeCount[m_i] != 0 )
		{
			m_end = m_i;
			break;
		}
	}

	//find the high intensity peak
	for( m_i = (m_start+1); m_i < (m_end-1); m_i++ )
	{
		if (m_VolumeCount[m_i] > m_high_freq )
		{
			m_high_freq = (int)m_VolumeCount[m_i];
			m_high_intensity = m_i;
		}
	}

	//find the low intensity peak
	for( m_i = (m_start+1); m_i < m_centre; m_i++ )
	{
		if ( m_VolumeCount[m_i] > m_low_freq )
		{
			m_low_freq = m_VolumeCount[m_i];
			m_low_intensity = m_i;
		}
	}

	// initialising the threshold variables to zero
	double max_diff = 0;

	unsigned int threshold_frequency = 0;

	//calculation of angle CDE 
	double alpha = atan( (double)(m_VolumeCount[m_high_intensity] - m_VolumeCount[m_low_intensity]) / ( m_high_intensity - m_low_intensity ));

	for( m_i = m_low_intensity; m_i < m_high_intensity; m_i++ )
	{
		//calculation of new height
		double new_height =  ((m_i - m_low_intensity) * tan(alpha)) + m_VolumeCount[m_low_intensity];

		// the difference between the new height and the original histogram height
		double diff = new_height - m_VolumeCount[m_i];

		//check whether the difference is greater than the previous difference
		//if YES change the max_difference value and the threshold intensity value
		//if NO check for the next intensity
		if ( max_diff < diff )
		{
			max_diff = diff;
			m_intensity = m_i;
			threshold_frequency = (unsigned int)m_VolumeCount[m_i];
		}
	}
	//0.5 is an appoxiemation factor 
	m_Threshold = (m_intensity + 0.5) * (this->m_Bins);

	m_BTIFFilter->SetLowerThreshold( 0 );
	m_BTIFFilter->SetUpperThreshold( m_Threshold );
	m_BTIFFilter->SetOutsideValue( 0 );
	m_BTIFFilter->SetInsideValue( MyThreshType<typename TImageType::PixelType>::Threshold );
	m_BTIFFilter->SetInput(this->GetInput());

	m_BTIFFilter->GraftOutput( this->GetOutput() );
	m_BTIFFilter->Update();
	this->GraftOutput( m_BTIFFilter->GetOutput() );
}

template<class TInputImage>
void iAMaximumDistanceFilter<TInputImage>::SetCentre(double lowIntensity)
{
	this->m_centre = (lowIntensity / (this->m_Bins));
}

template<class TInputImage>
void iAMaximumDistanceFilter<TInputImage>::SetBins(double bin)
{
	this->m_Bins = bin;
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetOutThreshold()
{
	return (int) (this->m_Threshold);
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetLowIntensity()
{
	return (int) ((this->m_low_intensity + 0.5) * (this->m_Bins));
}

template<class TInputImage>
int iAMaximumDistanceFilter<TInputImage>::GetHighIntensity()
{
	return (int) ((this->m_high_intensity + 0.5) * (this->m_Bins));
}

template <class TImageType>
void
	iAMaximumDistanceFilter<TImageType>::
	PrintSelf( std::ostream& os, itk::Indent indent ) const
{
	Superclass::PrintSelf(os,indent);

	os
		<< indent << "Threshold:" << this->m_Threshold
		<< std::endl;
}

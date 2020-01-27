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

#include <itkImageToImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkImageRegionConstIterator.h>

template <class TImageType>
class iAMaximumDistanceFilter :
	public itk::ImageToImageFilter<TImageType, TImageType>
{
public:
	typedef iAMaximumDistanceFilter                        Self;
	typedef itk::ImageToImageFilter<TImageType,TImageType> Superclass;
	typedef itk::SmartPointer<Self>                        Pointer;
	typedef itk::SmartPointer<const Self>                  ConstPointer;

	//! Method for creation through object factory
	itkNewMacro(Self);

	//! Run-time type information
	itkTypeMacro(iAMaximumDistanceFilter, itk::ImageToImageFilter);

	//! Display
	void PrintSelf( std::ostream& os, itk::Indent indent ) const override;

	typedef typename TImageType::PixelType PixelType;
	typedef typename TImageType::Pointer ImagePointer;
	typedef typename TImageType::ConstPointer ImageConstPointer;
	typedef itk::ImageRegionConstIterator <TImageType> ConstIteratorType;

	itkGetMacro( Threshold, PixelType);
	itkSetMacro( Threshold, PixelType);

	//! Set centre of histogram to calculate the low intensity peak.
	//! @param lowIntensity taken as the range to calculate the low intensity.
	void SetCentre(double lowIntensity);

	//! Set number of histogram bins.
	//! @param bin the number of bins in histogram.
	void SetBins(double bin);

	//! Get maximum distance threshold.
	//! @return t an int, which is the maximum distance threshold.
	int GetOutThreshold();

	//! Get low intensity peak.
	//!	@return an int, which is the low intensity peak.
	int GetLowIntensity();

	//! Get high intensity peak.
	//! @return an int, which is the high intensity peak.
	int GetHighIntensity();

protected:
	iAMaximumDistanceFilter();
	typedef itk::BinaryThresholdImageFilter< TImageType, TImageType > BTIFType;
	void GenerateData() override;

private:
	// prevent copying:
	iAMaximumDistanceFilter(Self&);
	void operator=(const Self&);

	typename BTIFType::Pointer     m_BTIFFilter;

	PixelType m_Threshold, m_intensity, m_first_value, m_last_value;
	PixelType m_i, m_start, m_end, m_high_intensity, m_low_intensity, m_centre;
	double m_Bins;
	int m_high_freq, m_low_freq;
	ImageConstPointer m_InImage;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAMaximumDistanceFilter.txx"
#endif

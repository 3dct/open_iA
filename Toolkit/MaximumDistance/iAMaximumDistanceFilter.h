// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageToImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkImageRegionConstIterator.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

//! Segmentation based on maximum distance of peaks in histogram
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

	//! Set the width of a bin in the histogram.
	//! @param binWidth the width of a bin in the histogram.
	void SetBinWidth(double binWidth);

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

	typename BTIFType::Pointer m_BTIFFilter;

	PixelType m_Threshold;
	int m_high_intensity, m_low_intensity, m_centre;
	double m_binWidth;
	ImageConstPointer m_inImage;
};

#include "iAMaximumDistanceFilter.txx"

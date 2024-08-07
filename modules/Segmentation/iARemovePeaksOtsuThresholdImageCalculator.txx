/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRemovePeaksOtsuThresholdImageCalculator.txx,v $
  Language:  C++
  Date:      $Date: 2006-03-28 23:46:10 $
  Version:   $Revision: 1.8 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkRemovePeaksOtsuThresholdImageCalculator_txx
#define _itkRemovePeaksOtsuThresholdImageCalculator_txx

#include <itkImageRegionConstIteratorWithIndex.h>
#include <itkMinimumMaximumImageCalculator.h>

#include "vnl/vnl_math.h"


template<class TInputImage>
iARemovePeaksOtsuThresholdImageCalculator<TInputImage>
::iARemovePeaksOtsuThresholdImageCalculator()
{
  m_Image = nullptr;
  m_Threshold = itk::NumericTraits<PixelType>::Zero;
  m_NumberOfHistogramBins = 128;
  m_RegionSetByUser = false;
}


//! Compute the Otsu's threshold
template<class TInputImage>
void
iARemovePeaksOtsuThresholdImageCalculator<TInputImage>
::Compute(void)
{
  if ( !m_Image ) { return; }
  if( !m_RegionSetByUser )
    {
    m_Region = m_Image->GetRequestedRegion();
    }

  //double totalPixels = (double) m_Region.GetNumberOfPixels();
  //if ( totalPixels == 0 ) { return; }

  // compute image max and min
  auto rangeCalculator = itk::MinimumMaximumImageCalculator<TInputImage>::New();
  rangeCalculator->SetImage( m_Image );
  rangeCalculator->Compute();

  PixelType imageMin = rangeCalculator->GetMinimum();
  PixelType imageMax = rangeCalculator->GetMaximum();

  if ( imageMin >= imageMax )
    {
    m_Threshold = imageMin;
    return;
    }

  // create a histogram
  std::vector<double> relativeFrequency;
  relativeFrequency.resize( m_NumberOfHistogramBins );
  for (size_t j = 0; j < m_NumberOfHistogramBins; j++ )
    {
    relativeFrequency[j] = 0.0;
    }

  double binMultiplier = (double) m_NumberOfHistogramBins /
    (double) ( imageMax - imageMin );

  itk::ImageRegionConstIteratorWithIndex<TInputImage> iter( m_Image, m_Region );

  while ( !iter.IsAtEnd() )
    {
    unsigned int binNumber;
    PixelType value = iter.Get();

    if ( value == imageMin )
      {
      binNumber = 0;
      }
    else
      {
      binNumber = (unsigned int) std::ceil((value - imageMin) * binMultiplier ) - 1;
      if ( binNumber == m_NumberOfHistogramBins ) // in case of rounding errors
        {
        binNumber -= 1;
        }
      }

    relativeFrequency[binNumber] += 1.0;
    ++iter;

    }

  // apply median filter to histogram
  double a, b, c;
  std::vector<double> temp;
  for (size_t j = 0; j < m_NumberOfHistogramBins; ++j )
    {
    (j == 0) ? a = 0 : a = relativeFrequency[j-1];
    b = relativeFrequency[j];
    (j == (m_NumberOfHistogramBins-1)) ? c = 0 : c = relativeFrequency[j+1];

    if      (((b <= c) && (b >= a)) || ((b <= a) && (b >= c)))
      temp.push_back(b);
    else if (((a <= c) && (a >= b)) || ((a <= b) && (a >= c)))
      temp.push_back(a);
    else if (((c <= b) && (c >= a)) || ((c <= a) && (c >= b)))
      temp.push_back(c);
  }

  double totalPixels = 0;
  for (size_t j = 0; j < m_NumberOfHistogramBins; ++j )
    {
    relativeFrequency[j] = temp[j];
    totalPixels += temp[j];
    }

  if ( totalPixels == 0 )
    {
    return;
    }

  // normalize the frequencies
  double totalMean = 0.0;
  for (size_t j = 0; j < m_NumberOfHistogramBins; j++ )
    {
    relativeFrequency[j] /= totalPixels;
    totalMean += (j+1) * relativeFrequency[j];
    }

  // compute Otsu's threshold by maximizing the between-class
  // variance
  double freqLeft = relativeFrequency[0];
  double meanLeft = 1.0;
  double meanRight = ( totalMean - freqLeft ) / ( 1.0 - freqLeft );

  double maxVarBetween = freqLeft * ( 1.0 - freqLeft ) *
#if ITK_VERSION_MAJOR < 5
    vnl_math_sqr( meanLeft - meanRight );
#else
    vnl_math::sqr( meanLeft - meanRight );
#endif
  size_t maxBinNumber = 0;

  double freqLeftOld = freqLeft;
  double meanLeftOld = meanLeft;

  for (size_t j = 1; j < m_NumberOfHistogramBins; j++ )
    {
    freqLeft += relativeFrequency[j];
    meanLeft = ( meanLeftOld * freqLeftOld +
                 (j+1) * relativeFrequency[j] ) / freqLeft;
    if (freqLeft == 1.0)
      {
      meanRight = 0.0;
      }
    else
      {
      meanRight = ( totalMean - meanLeft * freqLeft ) /
        ( 1.0 - freqLeft );
      }
    double varBetween = freqLeft * ( 1.0 - freqLeft ) *
      vnl_math::sqr( meanLeft - meanRight );

    if ( varBetween > maxVarBetween )
      {
      maxVarBetween = varBetween;
      maxBinNumber = j;
      }

    // cache old values
    freqLeftOld = freqLeft;
    meanLeftOld = meanLeft;

    }

  m_Threshold = static_cast<PixelType>( imageMin +
                                        ( maxBinNumber + 1 ) / binMultiplier );
}

template<class TInputImage>
void
iARemovePeaksOtsuThresholdImageCalculator<TInputImage>
::SetRegion( const RegionType & region )
{
  m_Region = region;
  m_RegionSetByUser = true;
}


template<class TInputImage>
void
iARemovePeaksOtsuThresholdImageCalculator<TInputImage>
::PrintSelf( std::ostream& os, itk::Indent indent ) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "Threshold: " << m_Threshold << std::endl;
  os << indent << "NumberOfHistogramBins: " << m_NumberOfHistogramBins << std::endl;
  os << indent << "Image: " << m_Image.GetPointer() << std::endl;
}

#endif

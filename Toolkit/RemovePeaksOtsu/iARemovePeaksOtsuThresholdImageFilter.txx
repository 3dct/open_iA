/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRemovePeaksOtsuThresholdImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2006-08-01 19:16:16 $
  Version:   $Revision: 1.4 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _iARemovePeaksOtsuThresholdImageFilter_txx
#define _iARemovePeaksOtsuThresholdImageFilter_txx

#include "iARemovePeaksOtsuThresholdImageCalculator.h"

#include "itkBinaryThresholdImageFilter.h"
#include "itkProgressAccumulator.h"

template<class TInputImage, class TOutputImage>
iARemovePeaksOtsuThresholdImageFilter<TInputImage, TOutputImage>
::iARemovePeaksOtsuThresholdImageFilter()
{
  m_OutsideValue   = itk::NumericTraits<OutputPixelType>::Zero;
  m_InsideValue    = itk::NumericTraits<OutputPixelType>::max();
  m_Threshold      = itk::NumericTraits<InputPixelType>::Zero;
  m_NumberOfHistogramBins = 128;
}

template<class TInputImage, class TOutputImage>
void
iARemovePeaksOtsuThresholdImageFilter<TInputImage, TOutputImage>
::GenerateData()
{
  auto progress = itk::ProgressAccumulator::New();
  progress->SetMiniPipelineFilter(this);

  // Compute the Otsu Threshold for the input image
  auto otsu = iARemovePeaksOtsuThresholdImageCalculator<TInputImage>::New();
  otsu->SetImage (this->GetInput());
  otsu->SetNumberOfHistogramBins (m_NumberOfHistogramBins);
  otsu->Compute();
  m_Threshold = otsu->GetThreshold();

  auto threshold = itk::BinaryThresholdImageFilter<TInputImage,TOutputImage>::New();

  progress->RegisterInternalFilter(threshold,.5f);
  threshold->GraftOutput (this->GetOutput());
  threshold->SetInput (this->GetInput());
  threshold->SetLowerThreshold(itk::NumericTraits<InputPixelType>::NonpositiveMin());
  threshold->SetUpperThreshold(otsu->GetThreshold());
  threshold->SetInsideValue (m_InsideValue);
  threshold->SetOutsideValue (m_OutsideValue);
  threshold->Update();

  this->GraftOutput(threshold->GetOutput());
}

template<class TInputImage, class TOutputImage>
void
iARemovePeaksOtsuThresholdImageFilter<TInputImage, TOutputImage>
::GenerateInputRequestedRegion()
{
  TInputImage * input = const_cast<TInputImage *>(this->GetInput());
  if( input )
    {
    input->SetRequestedRegionToLargestPossibleRegion();
    }
}

template<class TInputImage, class TOutputImage>
void 
iARemovePeaksOtsuThresholdImageFilter<TInputImage,TOutputImage>
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os,indent);

  os << indent << "OutsideValue: "
     << static_cast<typename itk::NumericTraits<OutputPixelType>::PrintType>(m_OutsideValue) << std::endl;
  os << indent << "InsideValue: "
     << static_cast<typename itk::NumericTraits<OutputPixelType>::PrintType>(m_InsideValue) << std::endl;
  os << indent << "NumberOfHistogramBins: "
     << m_NumberOfHistogramBins << std::endl;
  os << indent << "Threshold (computed): "
     << static_cast<typename itk::NumericTraits<InputPixelType>::PrintType>(m_Threshold) << std::endl;

}

#endif

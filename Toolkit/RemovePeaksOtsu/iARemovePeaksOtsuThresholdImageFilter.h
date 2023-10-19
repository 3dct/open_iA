/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRemovePeaksOtsuThresholdImageFilter.h,v $
  Language:  C++
  Date:      $Date: 2006-04-05 13:59:37 $
  Version:   $Revision: 1.2 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __iARemovePeaksOtsuThresholdImageFilter_h
#define __iARemovePeaksOtsuThresholdImageFilter_h

#include <itkImageToImageFilter.h>
#include <itkFixedArray.h>

//! Segment image using otsu threshold method, with additional "remove peak" functionality (by applying a median filter to the histogram)
//! 
//! Based on an (older version of the) itk::OtsuThresholdImageFilter
template<class TInputImage, class TOutputImage>
class iARemovePeaksOtsuThresholdImageFilter : 
	public itk::ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  /** Standard Self typedef */
  typedef iARemovePeaksOtsuThresholdImageFilter Self;
  typedef itk::ImageToImageFilter<TInputImage,TOutputImage>  Superclass;
  typedef itk::SmartPointer<Self>        Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);  

  /** Runtime information support. */
  itkTypeMacro(iARemovePeaksOtsuThresholdImageFilter, itk::ImageToImageFilter);
  
  /** Image pixel value typedef. */
  typedef typename TInputImage::PixelType   InputPixelType;
  typedef typename TOutputImage::PixelType   OutputPixelType;
  
  /** Image related typedefs. */
  typedef typename TInputImage::Pointer InputImagePointer;
  typedef typename TOutputImage::Pointer OutputImagePointer;

  typedef typename TInputImage::SizeType  InputSizeType;
  typedef typename TInputImage::IndexType  InputIndexType;
  typedef typename TInputImage::RegionType InputImageRegionType;
  typedef typename TOutputImage::SizeType  OutputSizeType;
  typedef typename TOutputImage::IndexType  OutputIndexType;
  typedef typename TOutputImage::RegionType OutputImageRegionType;


  /** Image related typedefs. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension ) ;
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension ) ;

  /** Set the "outside" pixel value. The default value 
   * NumericTraits<OutputPixelType>::Zero. */
  itkSetMacro(OutsideValue,OutputPixelType);
  
  /** Get the "outside" pixel value. */
  itkGetMacro(OutsideValue,OutputPixelType);

  /** Set the "inside" pixel value. The default value 
   * NumericTraits<OutputPixelType>::max() */
  itkSetMacro(InsideValue,OutputPixelType);
  
  /** Get the "inside" pixel value. */
  itkGetMacro(InsideValue,OutputPixelType);

  /** Set/Get the number of histogram bins. Defaults is 128. */
  itkSetClampMacro( NumberOfHistogramBins, unsigned long, 1, 
	  itk::NumericTraits<unsigned long>::max() );
  itkGetMacro( NumberOfHistogramBins, unsigned long );

  /** Get the computed threshold. */
  itkGetMacro(Threshold,InputPixelType);

#ifdef ITK_USE_CONCEPT_CHECKING

  /** Begin concept checking */
  itkConceptMacro(OutputEqualityComparableCheck,
    (itk::Concept::EqualityComparable<OutputPixelType>));
  itkConceptMacro(InputOStreamWritableCheck,
    (itk::Concept::OStreamWritable<InputPixelType>));
  itkConceptMacro(OutputOStreamWritableCheck,
    (itk::Concept::OStreamWritable<OutputPixelType>));
  /** End concept checking */
#endif
protected:
  iARemovePeaksOtsuThresholdImageFilter();
  ~iARemovePeaksOtsuThresholdImageFilter(){};
  void PrintSelf(std::ostream& os, itk::Indent indent) const override;

  void GenerateInputRequestedRegion() override;
  void GenerateData () override;

private:
	iARemovePeaksOtsuThresholdImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  InputPixelType      m_Threshold;
  OutputPixelType     m_InsideValue;
  OutputPixelType     m_OutsideValue;
  unsigned long       m_NumberOfHistogramBins;

} ; // end of class
  
#include "iARemovePeaksOtsuThresholdImageFilter.txx"

#endif

/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRemovePeaksOtsuThresholdImageCalculator.h,v $
  Language:  C++
  Date:      $Date: 2006-03-28 23:46:10 $
  Version:   $Revision: 1.7 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __iARemovePeaksOtsuThresholdImageCalculator_h
#define __iARemovePeaksOtsuThresholdImageCalculator_h

#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkNumericTraits.h"

//! Computes the Otsu's threshold for an image, with option to remove peaks.
//!
//! Based on https://itk.org/Doxygen/html/classitk_1_1OtsuThresholdImageFilter.html,
//! with additional option to remove
template <class TInputImage>
class iARemovePeaksOtsuThresholdImageCalculator : public itk::Object
{
public:
  //! @{ Standard class typedefs.
  typedef iARemovePeaksOtsuThresholdImageCalculator Self;
  typedef itk::Object  Superclass;
  typedef itk::SmartPointer<Self>   Pointer;
  typedef itk::SmartPointer<const Self>  ConstPointer;
  //! @}

  //! Method for creation through the object factory.
  itkNewMacro(Self);

  //! Run-time type information (and related methods).
  itkTypeMacro(iARemovePeaksOtsuThresholdImageCalculator, itk::Object);

  //! Type definition for the input image.
  typedef TInputImage  ImageType;

  //! Pointer type for the image.
  typedef typename TInputImage::Pointer  ImagePointer;

  //! Const Pointer type for the image.
  typedef typename TInputImage::ConstPointer ImageConstPointer;

  //! Type definition for the input image pixel type.
  typedef typename TInputImage::PixelType PixelType;

  //! Type definition for the input image region type.
  typedef typename TInputImage::RegionType RegionType;

  //! Set the input image.
  itkSetConstObjectMacro(Image,ImageType);

  //! Compute the Otsu's threshold for the input image.
  void Compute(void);

  //! Return the Otsu's threshold value.
  itkGetMacro(Threshold,PixelType);

  //! Set/Get the number of histogram bins. Default is 128.
  itkSetClampMacro( NumberOfHistogramBins, unsigned long, 1,
	  itk::NumericTraits<unsigned long>::max() );
  itkGetMacro( NumberOfHistogramBins, unsigned long );

  //! Set the region over which the values will be computed
  void SetRegion( const RegionType & region );

protected:
  iARemovePeaksOtsuThresholdImageCalculator();
  virtual ~iARemovePeaksOtsuThresholdImageCalculator() {};
  void PrintSelf(std::ostream& os, itk::Indent indent) const override;

private:
  iARemovePeaksOtsuThresholdImageCalculator(const Self&); //!< purposely not implemented
  void operator=(const Self&); //!< purposely not implemented

  PixelType            m_Threshold;
  unsigned long        m_NumberOfHistogramBins;
  ImageConstPointer    m_Image;
  RegionType           m_Region;
  bool                 m_RegionSetByUser;
  
};

#include "iARemovePeaksOtsuThresholdImageCalculator.txx"

#endif

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
#ifndef __itkRemovePeaksOtsuThresholdImageCalculator_h
#define __itkRemovePeaksOtsuThresholdImageCalculator_h

#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkNumericTraits.h"

namespace itk
{

/** \class RemovePeaksOtsuThresholdImageCalculator
 * \brief Computes the Otsu's threshold for an image.
 *
 * This calculator computes the Otsu's threshold which separates an image
 * into foreground and background components. The method relies on a
 * histogram of image intensities. The basic idea is to maximize the
 * between-class variance.
 *
 * This class is templated over the input image type.
 *
 * \warning This method assumes that the input image consists of scalar pixel
 * types.
 *
 * \ingroup Operators
 */
template <class TInputImage>
class RemovePeaksOtsuThresholdImageCalculator : public Object
{
public:
  /** Standard class typedefs. */
  typedef RemovePeaksOtsuThresholdImageCalculator Self;
  typedef Object  Superclass;
  typedef SmartPointer<Self>   Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RemovePeaksOtsuThresholdImageCalculator, Object);

  /** Type definition for the input image. */
  typedef TInputImage  ImageType;

  /** Pointer type for the image. */
  typedef typename TInputImage::Pointer  ImagePointer;

  /** Const Pointer type for the image. */
  typedef typename TInputImage::ConstPointer ImageConstPointer;

  /** Type definition for the input image pixel type. */
  typedef typename TInputImage::PixelType PixelType;

  /** Type definition for the input image region type. */
  typedef typename TInputImage::RegionType RegionType;

  /** Set the input image. */
  itkSetConstObjectMacro(Image,ImageType);

  /** Compute the Otsu's threshold for the input image. */
  void Compute(void);

  /** Return the Otsu's threshold value. */
  itkGetMacro(Threshold,PixelType);

  /** Set/Get the number of histogram bins. Default is 128. */
  itkSetClampMacro( NumberOfHistogramBins, unsigned long, 1,
                    NumericTraits<unsigned long>::max() );
  itkGetMacro( NumberOfHistogramBins, unsigned long );

  /** Set the region over which the values will be computed */
  void SetRegion( const RegionType & region );

protected:
  RemovePeaksOtsuThresholdImageCalculator();
  virtual ~RemovePeaksOtsuThresholdImageCalculator() {};
  void PrintSelf(std::ostream& os, Indent indent) const override;

private:
  RemovePeaksOtsuThresholdImageCalculator(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  PixelType            m_Threshold;
  unsigned long        m_NumberOfHistogramBins;
  ImageConstPointer    m_Image;
  RegionType           m_Region;
  bool                 m_RegionSetByUser;
  
};

} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkRemovePeaksOtsuThresholdImageCalculator.txx"
#endif

#endif

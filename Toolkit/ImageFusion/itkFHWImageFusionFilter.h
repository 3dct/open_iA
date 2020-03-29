/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkFHWImageFusionFilter.h,v $
  Language:  C++
  Date:      $Date: 2009-07-24 11:01:02 $
  Version:   $Revision: 1.00 $

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkFHWImageFusionFilter_h
#define __itkFHWImageFusionFilter_h

#include "itkInPlaceImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"

namespace itk
{
  
/** \class FHWImageFusionFilter
 * \brief Implements pixel-wise fusion of two images.
 *
 * This class is parameterized over the types of the four input images
 * and the type of the output image.  It is also parameterized by the
 * operation to be applied. 
 * 
 *
 * \ingroup IntensityImageFilters   Multithreaded
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4,
          class TOutputImage/*, class TFunction*/  >
class FHWImageFusionFilter :
    public InPlaceImageFilter<TInputImage1,TOutputImage> 
{
public:
  /** Standard class typedefs. */
  typedef FHWImageFusionFilter							 Self;
  typedef InPlaceImageFilter<TInputImage1,TOutputImage>  Superclass;
  typedef SmartPointer<Self>                             Pointer;
  typedef SmartPointer<const Self>                       ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro(FHWImageFusionFilter, InPlaceImageFilter);

  /** Some convenient typedefs. */
  //typedef TFunction                              FunctorType;
  typedef TInputImage1                           Input1ImageType;
  typedef typename Input1ImageType::ConstPointer Input1ImagePointer;
  typedef typename Input1ImageType::RegionType   Input1ImageRegionType; 
  typedef typename Input1ImageType::PixelType    Input1ImagePixelType; 

  typedef TInputImage2                           Input2ImageType;
  typedef typename Input2ImageType::ConstPointer Input2ImagePointer;
  typedef typename Input2ImageType::RegionType   Input2ImageRegionType; 
  typedef typename Input2ImageType::PixelType    Input2ImagePixelType; 

  typedef TInputImage3                           Input3ImageType;
  typedef typename Input3ImageType::ConstPointer Input3ImagePointer;
  typedef typename Input3ImageType::RegionType   Input3ImageRegionType; 
  typedef typename Input3ImageType::PixelType    Input3ImagePixelType; 

  typedef TInputImage4                           Input4ImageType;
  typedef typename Input4ImageType::ConstPointer Input4ImagePointer;
  typedef typename Input4ImageType::RegionType   Input4ImageRegionType; 
  typedef typename Input4ImageType::PixelType    Input4ImagePixelType; 

  typedef TOutputImage                           OutputImageType;
  typedef typename OutputImageType::Pointer      OutputImagePointer;
  typedef typename OutputImageType::RegionType   OutputImageRegionType;
  typedef typename OutputImageType::PixelType    OutputImagePixelType;

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput1( const TInputImage1 * image1);

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput2( const TInputImage2 * image2);

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput3( const TInputImage3 * image3);

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput4( const TInputImage4 * image4);

  /** Set the "EpsilonGradientMagnitude" pixel value. Default is 0. */
  itkSetMacro(EpsilonGradientMagnitude, double);
  /** Get the "EpsilonGradientMagnitude" pixel value. */
  itkGetMacro(EpsilonGradientMagnitude, double);

  /** Set the "EpsilonGreyValue" pixel value. Default is 0. */
  itkSetMacro(EpsilonGreyValue, double);
  /** Get the "EpsilonGreyValue" pixel value. */
  itkGetMacro(EpsilonGreyValue, double);

  /** Set the "TrustedDeltaGradientMagnitude" Default is 0. */
  itkSetMacro(TrustedDeltaGradientMagnitude, double);
  /** Get the "TrustedDeltaGradientMagnitude" pixel value. */
  itkGetMacro(TrustedDeltaGradientMagnitude, double);

  /** Set the "SigmoidMultiplier" pixel value. Default is 0. */
  itkSetMacro(SigmoidMultiplier, double);
  /** Get the "SigmoidMultiplier" pixel value. */
  itkGetMacro(SigmoidMultiplier, double);

    /** Get the "cnt2" value. */
  itkGetMacro(Cnt1, double);
    /** Get the "cnt2" value. */
  itkGetMacro(Cnt2, double);
    /** Get the "cnt3" value. */
  itkGetMacro(Cnt3, double);
    /** Get the "cnt" value. */
  itkGetMacro(Cnt, double);
  

  /** Set the functor object.  This replaces the current Functor with a
   * copy of the specified Functor. This allows the user to specify a
   * functor that has ivars set differently than the default functor.
   * This method requires an operator!=() be defined on the functor
   * (or the compiler's default implementation of operator!=() being
   * appropriate). */


  /** ImageDimension constants */
  itkStaticConstMacro(
    InputImage1Dimension, unsigned int, TInputImage1::ImageDimension);
  itkStaticConstMacro(
    InputImage2Dimension, unsigned int, TInputImage2::ImageDimension);
  itkStaticConstMacro(
    InputImage3Dimension, unsigned int, TInputImage3::ImageDimension);
  itkStaticConstMacro(
    InputImage4Dimension, unsigned int, TInputImage4::ImageDimension);
  itkStaticConstMacro(
    OutputImageDimension, unsigned int, TOutputImage::ImageDimension);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimensionCheck1,
    (Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
                            itkGetStaticConstMacro(OutputImageDimension)>));
  itkConceptMacro(SameDimensionCheck2,
    (Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
                            itkGetStaticConstMacro(InputImage2Dimension)>));
  itkConceptMacro(SameDimensionCheck3,
    (Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
                            itkGetStaticConstMacro(InputImage3Dimension)>));
  itkConceptMacro(SameDimensionCheck4,
    (Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
                            itkGetStaticConstMacro(InputImage4Dimension)>));
  /** End concept checking */
#endif

protected:
  FHWImageFusionFilter();
  virtual ~FHWImageFusionFilter() {}

  /** FHWImageFusionFilter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData() routine
   * which is called for each processing thread. The output image data is
   * allocated automatically by the superclass prior to calling
   * ThreadedGenerateData().  ThreadedGenerateData can only write to the
   * portion of the output image specified by the parameter
   * "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData()  */
  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
                            itk::ThreadIdType threadId ) override;

private:
  FHWImageFusionFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  //FunctorType m_Functor;
   double m_Cnt1, m_Cnt2, m_Cnt3, m_Cnt;
   double m_EpsilonGreyValue, m_EpsilonGradientMagnitude, m_TrustedDeltaGradientMagnitude, m_SigmoidMultiplier;
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkFHWImageFusionFilter.txx"
#endif

#endif

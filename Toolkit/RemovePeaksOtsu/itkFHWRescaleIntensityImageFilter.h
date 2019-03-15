/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __itkFHWRescaleIntensityImageFilter_h
#define __itkFHWRescaleIntensityImageFilter_h

#include "itkUnaryFunctorImageFilter.h"

namespace itk
{
// This functor class applies a linear transformation A.x + B
// to input values.
namespace Functor
{
template< typename TInput, typename  TOutput >
class FHWIntensityLinearTransform
{
public:
  typedef typename NumericTraits< TInput >::RealType RealType;
  FHWIntensityLinearTransform()
  {
    m_Factor = 1.0;
    m_Offset = 0.0;
    m_Minimum = NumericTraits< TOutput >::NonpositiveMin();
    m_Maximum = NumericTraits< TOutput >::max();
  }

  ~FHWIntensityLinearTransform() {}
  void SetFactor(RealType a) { m_Factor = a; }
  void SetOffset(RealType b) { m_Offset = b; }
  void SetMinimum(TOutput min) { m_Minimum = min; }
  void SetMaximum(TOutput max) { m_Maximum = max; }
  void SetInMinimum(TInput inmin) { m_InMinimum = inmin; }
  void SetInMaximum(TInput inmax) { m_InMaximum = inmax; }
  bool operator!=(const FHWIntensityLinearTransform & other) const
  {
    if ( m_Factor != other.m_Factor
         || m_Offset != other.m_Offset
         || m_Maximum != other.m_Maximum
         || m_Minimum != other.m_Minimum )
      {
      return true;
      }
    return false;
  }

  bool operator==(const FHWIntensityLinearTransform & other) const
  {
    return !( *this != other );
  }

  inline TOutput operator()(const TInput & x) const
  {
    RealType value  = static_cast< RealType >( x ) * m_Factor + m_Offset;
	value = ( value > m_Maximum ) ? m_Maximum : value;
    value = ( value < m_Minimum ) ? m_Minimum : value;

    TOutput  result = static_cast< TOutput >( value );

    result = ( result > m_Maximum ) ? m_Maximum : result;
    result = ( result < m_Minimum ) ? m_Minimum : result;
    return result;
  }

private:
  RealType m_Factor;
  RealType m_Offset;
  TInput m_InMinimum;
  TInput m_InMaximum;
  TOutput  m_Maximum;
  TOutput  m_Minimum;
};
}  // end namespace functor

/** \class FHWRescaleIntensityImageFilter
 * \brief Applies a linear transformation to the intensity levels of the
 * input Image.
 *
 * FHWRescaleIntensityImageFilter applies pixel-wise a linear transformation
 * to the intensity values of input image pixels. The linear transformation
 * is defined by the user in terms of the minimum and maximum values that
 * the output image should have.
 *
 * The following equation gives the mapping of the intensity values
 *
 * \par
 * \f[
 *  outputPixel = ( inputPixel - inputMin) \cdot
 *  \frac{(outputMax - outputMin )}{(inputMax - inputMin)} + outputMin
 * \f]
 *
 * All computations are performed in the precison of the input pixel's
 * RealType. Before assigning the computed value to the output pixel.
 */
template< typename  TInputImage, typename  TOutputImage = TInputImage >
class ITK_EXPORT FHWRescaleIntensityImageFilter:
  public
  UnaryFunctorImageFilter< TInputImage, TOutputImage,
                           Functor::FHWIntensityLinearTransform<
                             typename TInputImage::PixelType,
                             typename TOutputImage::PixelType >   >
{
public:
  /** Standard class typedefs. */
  typedef FHWRescaleIntensityImageFilter Self;
  typedef UnaryFunctorImageFilter<
    TInputImage, TOutputImage,
    Functor::FHWIntensityLinearTransform<
      typename TInputImage::PixelType,
      typename TOutputImage::PixelType > >  Superclass;

  typedef SmartPointer< Self >       Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef typename TOutputImage::PixelType                   OutputPixelType;
  typedef typename TInputImage::PixelType                    InputPixelType;
  typedef typename NumericTraits< InputPixelType >::RealType RealType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Runtime information support. */
  itkTypeMacro(FHWRescaleIntensityImageFilter,
               UnaryFunctorImageFilter);

  itkSetMacro(OutputMinimum, OutputPixelType);
  itkSetMacro(OutputMaximum, OutputPixelType);
  itkGetConstReferenceMacro(OutputMinimum, OutputPixelType);
  itkGetConstReferenceMacro(OutputMaximum, OutputPixelType);

  /** Get the Scale and Shift used for the linear transformation
      of gray level values.
   \warning These Values are only valid after the filter has been updated */
  itkGetConstReferenceMacro(Scale, RealType);
  itkGetConstReferenceMacro(Shift, RealType);

  /** Get the Minimum and Maximum values of the input image.
   \warning These Values are only valid after the filter has been updated */
  //itkGetConstReferenceMacro(InputMinimum, InputPixelType);
  //itkGetConstReferenceMacro(InputMaximum, InputPixelType);

  /** Process to execute before entering the multithreaded section */
  void BeforeThreadedGenerateData(void) override;

  /** Print internal ivars */
  void PrintSelf(std::ostream & os, Indent indent) const override;

  itkSetMacro(InputMinimum, float);
  itkSetMacro(InputMaximum, float);

#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro( InputHasNumericTraitsCheck,
                   ( Concept::HasNumericTraits< InputPixelType > ) );
  itkConceptMacro( OutputHasNumericTraitsCheck,
                   ( Concept::HasNumericTraits< OutputPixelType > ) );
  itkConceptMacro( RealTypeMultiplyOperatorCheck,
                   ( Concept::MultiplyOperator< RealType > ) );
  itkConceptMacro( RealTypeAdditiveOperatorsCheck,
                   ( Concept::AdditiveOperators< RealType > ) );
  /** End concept checking */
#endif
protected:
  FHWRescaleIntensityImageFilter();
  virtual ~FHWRescaleIntensityImageFilter() {}
private:
  FHWRescaleIntensityImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);              //purposely not implemented

  RealType m_Scale;
  RealType m_Shift;

  InputPixelType m_InputMinimum;
  InputPixelType m_InputMaximum;

  OutputPixelType m_OutputMinimum;
  OutputPixelType m_OutputMaximum;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkFHWRescaleIntensityImageFilter.txx"
#endif

#endif

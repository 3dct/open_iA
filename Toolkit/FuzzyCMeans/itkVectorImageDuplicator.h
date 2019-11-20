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
#ifndef __itkVectorImageDuplicator_h
#define __itkVectorImageDuplicator_h

#include "itkObject.h"
#include "itkImage.h"

namespace itk
{

/** \class VectorImageDuplicator
 * \brief This helper class create a vector image which is perfect copy of the
 * input vector image.
 *
 * This class is NOT a filter. Although it has an API similar to a filter, this
 * class is not intended to be used in a pipeline. Instead, the typical use
 * will be like it is illustrated in the following code:
 *
 * \code
 *     medianFilter->Update();
 *     VectorImageType::Pointer vectorImage = medianFilter->GetOutput();
 *     typedef VectorImageDuplicator< VectorImageType > DuplicatorType;
 *     DuplicatorType::Pointer duplicator = DuplicatorType::New();
 *     duplicator->SetInput();
 *     duplicator->Update();
 *     ImageType::Pointer clonedVectorImage = duplicator->GetOutput();
 * \endcode
 *
 * Note that the Update() method must be called explicitly in the filter
 * that provides the input to the VectorImageDuplicator object. This is needed
 * because the VectorImageDuplicator is not a pipeline filter.
 *
 */
template <class TInputVectorImage>
class VectorImageDuplicator : public Object
{
public:
  /** Standard class typedefs. */
  typedef VectorImageDuplicator         Self;
  typedef Object                        Superclass;
  typedef SmartPointer<Self>            Pointer;
  typedef SmartPointer<const Self>      ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(VectorImageDuplicator, Object);

  /** Type definitions for the input image. */
  typedef TInputVectorImage                             ImageType;
  typedef typename TInputVectorImage::Pointer           ImagePointer;
  typedef typename TInputVectorImage::ConstPointer      ImageConstPointer;
  typedef typename TInputVectorImage::PixelType         PixelType;
  typedef typename TInputVectorImage::InternalPixelType InternalPixelType;
  typedef typename TInputVectorImage::IndexType         IndexType;

  itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);

  /** Set the input image. */
  itkSetConstObjectMacro(InputImage,ImageType);

  /** Get the output image. */
  itkGetObjectMacro(Output,ImageType);

  /** Compute of the input image. */
  void Update(void);

protected:
  VectorImageDuplicator();
  virtual ~VectorImageDuplicator() {};
  void PrintSelf(std::ostream& os, Indent indent) const override;

private:
  VectorImageDuplicator(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  ImageConstPointer       m_InputImage;
  ImagePointer            m_Output;
  unsigned long           m_InternalImageTime;

};

} // end namespace itk

// Define instantiation macro for this template.
#define ITK_TEMPLATE_VectorImageDuplicator(_, EXPORT, x, y) namespace itk { \
  _(1(class EXPORT VectorImageDuplicator< ITK_TEMPLATE_1 x >)) \
  namespace Templates { typedef VectorImageDuplicator< ITK_TEMPLATE_1 x > VectorImageDuplicator##y; } \
  }

#if ITK_TEMPLATE_EXPLICIT
# include "Templates/itkVectorImageDuplicator+-.h"
#endif

#if ITK_TEMPLATE_TXX
# include "itkVectorImageDuplicator.txx"
#endif

#endif /* __itkVectorImageDuplicator_h */

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
#ifndef __itkFuzzyClassifierImageFilter_h
#define __itkFuzzyClassifierImageFilter_h

#include "itkImageToImageFilter.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

#if ITK_VERSION_MAJOR < 4
#ifdef ITK_USE_REVIEW_STATISTICS
#include "itkMaximumDecisionRule2.h"
#else
#include "itkMaximumDecisionRule.h"
#endif
#else
#include "itkMaximumDecisionRule.h"
#endif

namespace itk
{

/** \class FuzzyClassifierImageFilter
 *
 * \brief Performs defuzzification on a membership image.
 *
 * The filter implements the defuzzification (i.e., the assignment of each
 * pixel to a particular class based on their memberships) by applying a
 * Maximum Decision Rule to the membership image.
 *
 * If a pixel of the membership image has values -1, that pixel is assumed to
 * be a background pixel and should be ignored. The value of the label
 * assigned to those pixels will be equal to the number of classes. Otherwise,
 * the pixel values of the resulting label map will indicate the classes they
 * correspond to (e.g. labels with value 0 belong to the class 0, labels with
 * value 1 belong to the first class, etc.). Thus the range of label values is
 * 0 to the number of classes.
 *
 * \par Inputs and Outputs
 * The input to this filter is an itk::VectorImage that represents pixel
 * memberships to 'n' classes. You may use a subclass of
 * FuzzyClassifierInitializationImageFilter to generate the membership images
 * or specify your own.
 *
 * \par
 * The output of the filter is a label map (an image of unsigned char is the
 * default).
 *
 * \par Template parameters
 * This filter is templated over the input vector image type and the data type
 * of the output label map (defaults to unsigned char).
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa FuzzyClassifierInitializationImageFilter
 * \sa VectorImage
 *
 * \ingroup ClassificationFilters
*/
template< class TInputVectorImage, class TLabel = unsigned char >
class FuzzyClassifierImageFilter :
    public ImageToImageFilter<
               TInputVectorImage,
               Image< TLabel,
                      TInputVectorImage::ImageDimension > >
{

public:

  /** Convenient typedefs for simplifying declarations. */
  typedef TInputVectorImage                                 InputImageType;
  typedef TLabel                                            LabelType;


  /** Extract dimension from input image. */
  itkStaticConstMacro( InputImageDimension, unsigned int,
                       InputImageType::ImageDimension );


  /** Standard class typedefs. */
  typedef FuzzyClassifierImageFilter                            Self;

  /** Convenient typedefs for simplifying declarations. */
  typedef Image< LabelType,
              itkGetStaticConstMacro(InputImageDimension) > OutputImageType;

  /** Standard class typedefs. */
  typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef SmartPointer< Self >                                  Pointer;
  typedef SmartPointer< const Self >                            ConstPointer;


  /** Input image typedef support (types inherited from the superclass). Input
   * vector image type representing the membership of a pixel to a particular
   * class. This image has arrays as pixels, the number of elements in the
   * array is the same as the number of classes to be used. */
  typedef typename Superclass::InputImagePointer      InputImagePointer;
  typedef typename Superclass::InputImageConstPointer InputImageConstPointer;
  typedef typename Superclass::InputImageRegionType   InputImageRegionType;
  typedef typename Superclass::InputImagePixelType    InputImagePixelType;

  /** Other input image typedefs. */
  typedef ImageRegionConstIterator< InputImageType > InputImageConstIterator;

  /** Output image typedef support (types inherited from the superclass).
   * The value of each element in the label map is the label of
   * the winner class for the corresponding pixel of the input image.
   * A negative value in the matrix indicates that the corresponding
   * pixel of the input image is ignored (i.e. it's a background value). */
  typedef typename Superclass::OutputImagePointer    OutputImagePointer;
  typedef typename Superclass::OutputImageRegionType OutputImageRegionType;
  typedef typename Superclass::OutputImagePixelType  OutputImagePixelType;

  /** Other output image typedefs. */
  typedef ImageRegionIterator< OutputImageType > OutputImageIterator;

  /** Decision rule used in the classification process to compare the
   * membership scores and return a class label. */
#if ITK_VERSION_MAJOR < 4
#ifdef ITK_USE_REVIEW_STATISTICS
  typedef itk::Statistics::MaximumDecisionRule2 DecisionRuleType;
#else
  typedef itk::MaximumDecisionRule              DecisionRuleType;
#endif
#else
  typedef itk::Statistics::MaximumDecisionRule              DecisionRuleType;
#endif
  typedef DecisionRuleType::Pointer             DecisionRulePointer;


  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(FuzzyClassifierImageFilter, ImageToImageFilter);


#ifdef ITK_USE_CONCEPT_CHECKING
  /* Begin concept checking. */
  itkConceptMacro( UnsignedIntConvertibleToLabelCheck,
                   (Concept::Convertible< unsigned int, LabelType >) );
  itkConceptMacro( InputHasNumericTraitsCheck,
                   ( Concept::HasNumericTraits<
                         typename InputImagePixelType::ValueType > ) );
  /* End concept checking. */
#endif


protected:

  /** Constructor. */
  FuzzyClassifierImageFilter() {}

  /** Destructor. */
  virtual ~FuzzyClassifierImageFilter() {}

  /** Write the name-value pairs of the filter data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, Indent indent) const override;


  /** Standard pipeline method. Here is where the classification is
   * performed. */
  void GenerateData() override;

  /** This method computes the labeled map. */
  virtual void Classify();


private:

  /** Copy constructor. Purposely not implemented. */
  FuzzyClassifierImageFilter(const Self&);

  /** Basic assignment operator. Purposely not implemented. */
  void operator=(const Self&);

}; // end of class

} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkFuzzyClassifierImageFilter.txx"
#endif

#endif // __itkFuzzyClassifierImageFilter_h

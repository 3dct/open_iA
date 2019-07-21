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
#ifndef __itkFuzzyClassifierInitializationImageFilter_h
#define __itkFuzzyClassifierInitializationImageFilter_h

#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkImageToImageFilter.h"
#include "itkNumericTraits.h"
#include "itkPixelTraits.h"
#include "itkMacro.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkVector.h"
#include "itkArray.h"
#if ITK_VERSION_MAJOR >= 5
#include "itkMultiThreaderBase.h"
#else
#include "itkMultiThreader.h"
#endif
#include <vector>

namespace itk
{

/** \class FuzzyClassifierInitializationImageFilter
 *
 * \brief Base class for fuzzy classifiers that take an image as input and
 * produce a membership itk::VectorImage as output.
 *
 * FuzzyClassifierInitializationImageFilter class provides the infrastructure
 * for supporting multithread. This class provides a GenerateData() method
 * based on the superclass (ImageSource) implementation. If a derived class
 * provides an implementation of ThreadedGenerateData(), this implementation
 * of GenerateData() allocates the output buffer and an image used internally.
 * It also calls the method called Initialize(). This method is used by
 * derived classes that need to initialize some data or perform some
 * calculations before to start the first iteration. Then it performs the
 * iterations of the fuzzy algorithm. At each iteration, the
 * BeforeThreadedGenerateData() method is called (if provided). Then, the
 * image is divided into a number of pieces and a number of threads are
 * spawned each calling ThreadedGenerateData(). After all the threads have
 * completed processing, the AfterThreadedGenerateData() method is called (if
 * provided). To halt the iteration process the error (usually the difference
 * between the new values of the centroids and the previous values) must lie
 * below a determined threshold or the number of iteration must exceed the
 * maximum number of iterations, otherwise a new iteration will start.
 *
 * If a derived class provides an implementation of GenerateData() instead,
 * the fuzzy algorithm will run in a single thread and the implementation is
 * responsible for allocating its output data.
 *
 * \par Inputs and Outputs
 * The input to this filter is the image to classify.
 *
 * \par
 * The output of the filter is an itk::VectorImage, that represents pixel
 * memberships to 'n' classes.
 *
 * \par Template parameters
 * This filter is templated over the input image type, the data type used to
 * represent the probabilities (defaults to double) and the data type used to
 * represent the values of the centroids (defaults to double). Note that the
 * precision types used to instantiate this filter should be real valued
 * scalar types. In other words: doubles or floats.
 *
 * \par Internal image
 * This class uses an internal image to speed up execution. This image is
 * obtained by a numeric conversion of the pixels of the input image to the
 * type of centroids. This will avoid doing a conversion each time the
 * algorithm accesses the pixels.
 *
 * \par Caveats
 * When using Iterators on the output vector image, you cannot use the
 * it.Value(). You must use Set/Get() methods instead.
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa FuzzyClassifierImageFilter
 * \sa FCMClassifierInitializationImageFilter
 * \sa KFCMSClassifierInitializationImageFilter
 * \sa MSKFCMClassifierInitializationImageFilter
 * \sa MSFKCMClassifierInitializationImageFilter
 * \sa VectorImage
 *
 * \ingroup ClassificationFilters
*/
template< class TInputImage, class TProbabilityPrecision = double,
          class TCentroidValuePrecision = double >
class ITK_EXPORT FuzzyClassifierInitializationImageFilter :
    public ImageToImageFilter<
               TInputImage,
               VectorImage<
                   TProbabilityPrecision,
                   TInputImage::ImageDimension > >
{

public:

  /** Convenient typedefs for simplifying declarations. */
  typedef TInputImage                                     InputImageType;
  typedef TProbabilityPrecision                           MembershipValueType;
  typedef TCentroidValuePrecision                         CentroidValueType;


  /** Extract dimension from input image. */
  itkStaticConstMacro( InputImageDimension, unsigned int,
                       InputImageType::ImageDimension );


  /** Standard class typedefs. */
  typedef FuzzyClassifierInitializationImageFilter              Self;

  /** Convenient typedefs for simplifying declarations. */
  typedef VectorImage< MembershipValueType,
              itkGetStaticConstMacro(InputImageDimension) > OutputImageType;

  /** Standard class typedefs. */
  typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef SmartPointer< Self >                                  Pointer;
  typedef SmartPointer< const Self >                            ConstPointer;


  /** Input image typedef support (types inherited from the superclass). */
  typedef typename Superclass::InputImagePointer InputImagePointer;

  typedef typename Superclass::InputImageConstPointer InputImageConstPointer;
  typedef typename Superclass::InputImageRegionType   InputImageRegionType;
  typedef typename Superclass::InputImagePixelType    InputImagePixelType;

  /** Other input image typedefs. */
  typedef typename InputImageType::SizeType          InputImageSizeType;
  typedef typename InputImageType::IndexType         InputImageIndexType;
  typedef ImageRegionConstIterator< InputImageType > InputImageConstIterator;

  /** Input image pixel typedefs. */
  typedef NumericTraits< InputImagePixelType >
                                             InputImagePixelNumericTraitsType;
  typedef PixelTraits< InputImagePixelType > InputImagePixelTraitsType;
  typedef typename InputImagePixelTraitsType::ValueType
                                             InputImagePixelValueType;


  /** Extract dimension from the pixel of the input image. */
  itkStaticConstMacro( InputImagePixelDimension, unsigned int,
                       InputImagePixelTraitsType::Dimension );


  /** Centroid typedef. The type of the components of the centroid is the
   * template parameter TCentroidValuePrecision (i.e. CentroidValueType). */
  typedef typename itk::Vector< CentroidValueType, InputImagePixelDimension >
    CentroidType;

  /** Typdef for the array of centroids. */
  typedef std::vector< CentroidType > CentroidArrayType;

  /** Other centroid typedefs. */
  typedef NumericTraits< CentroidValueType >
    CentroidValueNumericTraitsType;

  /** Internal image typedef support. This image is used for internal
   * processing. It's obtained by converting the pixels of the input image to
   * the type of centroid. */
  typedef Image< CentroidType, itkGetStaticConstMacro(InputImageDimension) >
                                                   InternalImageType;
  typedef typename InternalImageType::Pointer      InternalImagePointer;
  typedef typename InternalImageType::ConstPointer InternalImageConstPointer;

  /** Other internal image typedefs. */
  typedef ImageRegionIterator< InternalImageType > InternalImageIterator;
  typedef ImageRegionConstIterator< InternalImageType >
                                                   InternalImageConstIterator;

  /** Membership image typedef support. The pixels of the vector image
   * represent the membership of that pixel to each particular class. The
   * pixels are arrays and the number of elements in the array is the same as
   * the number of classes to be used. */
  typedef OutputImageType                            MembershipImageType;
  typedef typename MembershipImageType::PixelType    MembershipImagePixelType;
  typedef typename MembershipImageType::Pointer      MembershipImagePointer;
  typedef ImageRegionIterator< MembershipImageType > MembershipImageIterator;


  /** Run-time type information (and related methods). */
  itkTypeMacro(FuzzyClassifierInitializationImageFilter, ImageToImageFilter);


  /** Get the number of classes. */
  itkGetConstMacro(NumberOfClasses, unsigned int);

  /** Set the number of classes. This method also sets the size of the vector
   * of centroids. The user must supply it. */
  virtual void SetNumberOfClasses(const unsigned int &numOfClasses);

  /** Get cluster centroids. */
  virtual CentroidArrayType GetCentroids() const;

  /** Set class centroids.
   * The number of centroids must equal the number of classes. */
  virtual void SetCentroids(const CentroidArrayType newCentroids);

  /** Get maximum number of iterations. */
  itkGetConstMacro(MaximumNumberOfIterations, unsigned int);

  /** Set maximum number of iterations.
   * This value should be positive and greater than 1. */
  itkSetClampMacro( MaximumNumberOfIterations, unsigned int, 1,
                    NumericTraits< unsigned int >::max() );

  /** Get number of iterations performed. */
  itkGetConstMacro(NumberOfIterations, unsigned int);

  /** Get maximum error used to stop the algorithm. */
  itkGetConstMacro(MaximumError, double);

  /** Set maximum error used to stop the algorithm.
   * This error value can't be negative. */
  itkSetClampMacro(MaximumError, double, 0.0, NumericTraits< double >::max());

  /** Get error between the new class centroids and their values from
   * the previous iteration. */
  itkGetConstMacro(Error, double);

  /** Get m fuzziness degree value. */
  itkGetConstMacro(M, double);

  /** Set m fuzziness degree value. This value should be greater than 1 . */
  itkSetClampMacro(M, double, 1.0, NumericTraits< double >::max());

  /** Get true/false depending on whether the algorithm ignores
   * the background pixels or not. */
  itkGetConstMacro(IgnoreBackgroundPixels, bool);

  /** Set true/false depending on whether the algorithm ignores
   * the background pixels or not. */
  itkSetMacro(IgnoreBackgroundPixels, bool);

  /** Turn on and off the IgnoreBackgroundPixels flag. */
  itkBooleanMacro(IgnoreBackgroundPixels);

  /** Get the pixel used as background value in the input image. */
  itkGetConstMacro(BackgroundPixel, InputImagePixelType);

  /** Set the pixel used as background value in the input image.
   * The image pixels with this value will be ignored. */
  itkSetMacro(BackgroundPixel, InputImagePixelType);


  /** Computes the mean square error between centroids and the centroid array
   * passed as parameter. */
  virtual double ComputeDifference(const CentroidArrayType &arrayOfCentroids);
  

#ifdef ITK_USE_CONCEPT_CHECKING
  /* Begin concept checking. */
  itkConceptMacro( DoubleConvertibleToInputPixelValueCheck,
                 (Concept::Convertible< double, InputImagePixelValueType >) );
  itkConceptMacro( DoubleConvertibleToMembershipValueCheck,
                   (Concept::Convertible< double, MembershipValueType >) );
  itkConceptMacro( DoubleConvertibleToCentroidValueCheck,
                   (Concept::Convertible< double, CentroidValueType >) );
  itkConceptMacro( InputHasNumericTraitsCheck,
                   (Concept::HasNumericTraits< InputImagePixelValueType >) );
  itkConceptMacro( MembershipValueHasNumericTraitsCheck,
                   (Concept::HasNumericTraits< MembershipValueType >) );
  itkConceptMacro( CentroidValueHasNumericTraitsCheck,
                   (Concept::HasNumericTraits< CentroidValueType >) );
  itkConceptMacro( DoublePlusCentroidValueCheck,
                   ( Concept::AdditiveOperators< CentroidValueType,
                                                 CentroidValueType,
                                                 double > ) );
  itkConceptMacro( DoubleTimesCentroidValueCheck,
                   ( Concept::MultiplyOperator< CentroidValueType,
                                                CentroidValueType,
                                                double > ) );
  /* End concept checking. */
#endif


protected:

  /** Constructor. */
  FuzzyClassifierInitializationImageFilter();

  /** Destructor. */
  virtual ~FuzzyClassifierInitializationImageFilter() {}

  /** Write the name-value pairs of the filter data members to the
   * supplied output stream. */
  void PrintSelf(std::ostream &os, Indent indent) const override;


  /** Generate the information describing the output data. The default
   * implementation of this method will copy information from the input to the
   * output. The filter calls that superclass' implementation of this method
   * and then it sets the length of the output image pixel (i.e., the array of
   * memberships) to the number of classes. */
  void GenerateOutputInformation() override;

  /** Transforms a given pixel of the input image to the equivalent
   * pixel of centroid type. */
  inline void ComputePixelAsCentroid( const InputImagePixelType &pixel,
                                      CentroidType &pixelAsCentroid );

  /** Get the internal image to process. This image will be used for
   * internal processing and is obtained from the original input image
   * by transforming its pixels to the type of centroids. */
  void ComputeImageToProcess(InternalImageType *imageToProcess);

  /** If a subclass of FuzzyClassifierInitializationImageFilter needs to
   * initialize some data or perform some calculations before to start the
   * first iteration, it should provide an implementation of Initialize(). */
  virtual void Initialize() {}

  /** A version of GenerateData() specific for fuzzy classifiers. This
   * implementation will split the processing across multiple threads. The
   * buffer is allocated by this method. It also calls Initialize(). Then it
   * performs the iterations of the fuzzy algorithm. At each iteration the
   * BeforeThreadedGenerateData() method is called (if provided). Then, the
   * image is divided into a number of pieces and a number of threads are
   * spawned each calling ThreadedGenerateData(). After all the threads have
   * completed processing, the AfterThreadedGenerateData() method is called
   * (if provided). To halt the iteration process the error (i.e., the
   * difference between the new values of the centroids and the previous
   * values) must lie below a determined thresold or the number of iteration
   * must exceed the maximum number of iterations, otherwise a new iteration
   * will start. If a derived class cannot be threaded, that class should
   * provide an implementation of GenerateData(). That implementation is
   * responsible for allocating the output buffer. If a subclass can be
   * threaded, it should NOT provide a GenerateData() method but should
   * provide a ThreadedGenerateData() instead.
   *
   * \sa ThreadedGenerateData() */
  void GenerateData() override;


  /** Internal structure used for passing image data into the threading
   * library. */
  struct ThreadStruct
    {
    Pointer Filter;
    };


  /** Number of fuzzy clusters used to classify the image. The number of
   * classes will determine the number of membership images that will be
   * generated. The user must specify the number of expected classes before
   * calling the classifier. */
  unsigned int m_NumberOfClasses;

  /** Array of class centroids. */
  CentroidArrayType m_Centroids;

  /** Maximum number of iterations allowed. */
  unsigned int m_MaximumNumberOfIterations;

  /** Number of iterations performed. During the execution of the algorithm it
   * represents the current iteration. */
  unsigned int m_NumberOfIterations;

  /** Error threshold used to stop the algorithm. */
  double m_MaximumError;

  /** Error obtained between the new class centroids and the old values of the
   * centroids (i.e. the values in the previous iteration). During the
   * execution of the algorithm it represents the current error and it's
   * updated at each iteration. */
  double m_Error;

  /** Fuzzification parameter (m) in the range [1, n], which determines the
   * degree of fuzziness in the clusters. When m = 1 the effect is a crisp
   * clustering of points. When m > 1 the degree of fuzziness among points in
   * the decision space increases. */
  double m_M;

  /** Flag indicating whether to ignore the background pixels. */
  bool m_IgnoreBackgroundPixels;

  /** Pixel of the input image used as background value. */
  InputImagePixelType m_BackgroundPixel;

  /** Pointer to the image to be used internally by the algorithm. This image
   * is obtained by a numeric conversion of the pixels of the input image to
   * the type of centroids. */
  InternalImagePointer m_ImageToProcess;


private:

  /** Copy constructor. Purposely not implemented. */
  FuzzyClassifierInitializationImageFilter(const Self&);

  /** Basic assignment operator. Purposely not implemented. */
  void operator=(const Self&);

}; // end of class

} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkFuzzyClassifierInitializationImageFilter.txx"
#endif

#endif // __itkFuzzyClassifierInitializationImageFilter_h

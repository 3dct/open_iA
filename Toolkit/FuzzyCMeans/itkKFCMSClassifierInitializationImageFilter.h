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
#ifndef __itkKFCMSClassifierInitializationImageFilter_h
#define __itkKFCMSClassifierInitializationImageFilter_h

#include "itkFuzzyClassifierInitializationImageFilter.h"
#include "itkRBFKernelInducedDistanceMetric.h"
#include "itkFlatStructuringElement.h"

#include "itkMacro.h"
#include "itkVector.h"
#include "itkArray.h"
#include "itkConstShapedNeighborhoodIterator.h"
#include "itkNumericTraits.h"
#if ITK_VERSION_MAJOR >= 5
#include <mutex>
#else
#include "itkFastMutexLock.h"
#endif
#include <vector>

namespace itk
{

/** \class KFCMSClassifierInitializationImageFilter
 *
 * \brief Performs Spatially Constrained Fuzzy C-Means based on kernel-induced distance (KFCMS)
 * Classification on an image.
 *
 * This implementation is based on S. C. Chen. and D. Q. Zhang, "Robust image segmentation using
 * FCM with spatial constraints based on new kernel-induced distance measure". Systems, Man, and
 * Cybernetics, Part B: Cybernetics, IEEE Transactions on, 34(4):1907–1916, 2004. 1, 2.2
 *
 * KFCMSClassifierInitializationImageFilter is implemented as a multithreaded
 * filter. This implementation provides BeforeThreadedGenerateData(),
 * ThreadedGenerateData() and AfterThreadedGenerateData() methods.
 * Documentation of the superclass (FuzzyClassifierInitializationImageFilter)
 * explains multithreaded support in fuzzy classifiers.
 *
 * \par Caveats
 * The user should set the RBF kernel distance metric.
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa FuzzyClassifierInitializationImageFilter
 * \sa FCMClassifierInitializationImageFilter
 * \sa MSKFCMClassifierInitializationImageFilter
 * \sa MSFKCMClassifierInitializationImageFilter
 * \sa FuzzyClassifierImageFilter
 *
 * \ingroup ClassificationFilters
 */
template< class TInputImage, class TProbabilityPrecision = double,
          class TCentroidValuePrecision = double >
class KFCMSClassifierInitializationImageFilter :
    public FuzzyClassifierInitializationImageFilter< TInputImage,
                                                     TProbabilityPrecision,
                                                     TCentroidValuePrecision >
{

public:

  /** Standard class typedefs. */
  typedef KFCMSClassifierInitializationImageFilter  Self;
  typedef FuzzyClassifierInitializationImageFilter<
              TInputImage,
              TProbabilityPrecision,
              TCentroidValuePrecision >             Superclass;
  typedef SmartPointer< Self >                      Pointer;
  typedef SmartPointer< const Self >                ConstPointer;

  /** Input image typedef support (types inherited from the superclass). */
  typedef typename Superclass::InputImageType       InputImageType;
  typedef typename Superclass::InputImageRegionType InputImageRegionType;
  typedef typename Superclass::InputImagePixelType  InputImagePixelType;
  typedef typename Superclass::InputImagePixelValueType
                                                    InputImagePixelValueType;
  typedef typename Superclass::InputImageSizeType   InputImageSizeType;
  typedef typename Superclass::InputImageIndexType  InputImageIndexType;

  /** Extract dimension from input image. */
  itkStaticConstMacro( InputImageDimension, unsigned int,
                       InputImageType::ImageDimension );

  /** Centroid typedefs (types inherited from the superclass). */
  typedef typename Superclass::CentroidType      CentroidType;
  typedef typename Superclass::CentroidValueType CentroidValueType;
  typedef typename Superclass::CentroidArrayType CentroidArrayType;

  /** Other centroid typedefs. */
  typedef NumericTraits< CentroidType >      CentroidNumericTraitsType;
  typedef NumericTraits< CentroidValueType > CentroidValueNumericTraitsType;

  /** Internal image typedef support (types inherited from the superclass).
   * This image is used for internal processing. */
  typedef typename Superclass::InternalImageType    InternalImageType;
  typedef typename Superclass::InternalImagePointer InternalImagePointer;
  typedef typename Superclass::InternalImageConstPointer
                                                    InternalImageConstPointer;

  /** Other internal image typedefs. */
  typedef ConstShapedNeighborhoodIterator< InternalImageType >
    InternalImageConstShapedNeighborhoodIterator;

  /** Membership matrix typedef support (types inherited from the
   * superclass). */
  typedef typename Superclass::MembershipImageType    MembershipImageType;
  typedef typename Superclass::MembershipValueType    MembershipValueType;
  typedef typename Superclass::MembershipImagePixelType
                                                      MembershipImagePixelType;
  typedef typename Superclass::MembershipImagePointer MembershipImagePointer;
  typedef typename Superclass::MembershipImageIterator
                                                      MembershipImageIterator;

  /** Other membership typedefs. */
  typedef typename MembershipImageType::RegionType OutputImageRegionType;
  typedef NumericTraits< MembershipValueType >
    MembershipValueNumericTraitsType;

  /** Type definitions for kernel distance metric measure to be used. */
  typedef Statistics::KernelInducedDistanceMetric< CentroidType >
    KernelDistanceMetricType;
  typedef typename KernelDistanceMetricType::Pointer
    KernelDistanceMetricPointer;

  /** Type definitions for mutex lock. Mutex lock allows the locking of
   * variables which are accessed through different threads. */
#if ITK_VERSION_MAJOR >= 5
  typedef std::mutex MutexLockType;
#else
  typedef FastMutexLock MutexLockType;
#endif

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro( KFCMSClassifierInitializationImageFilter,
                FuzzyClassifierInitializationImageFilter );


  /** Set number of classes. This method calls
   * Superclass::SetNumberOfClasses(numOfClasses) and creates some variables
   * used for centroids calculations. This method also sets the size of the
   * vector of old centroids.*/
  void SetNumberOfClasses(const unsigned int &numOfClasses) override;

  /** Get penalty factor of the neighborhood. */
  itkGetConstMacro(Alpha, double);

  /** Set penalty factor of the neighborhood.
   * This value should be greater than 0. */
  itkSetClampMacro(Alpha, double, 0.0, NumericTraits< double >::max());

  /** Structuring element typedef. */
  typedef typename itk::FlatStructuringElement<
  itkGetStaticConstMacro(InputImageDimension) > StructuringElementType;

  typedef typename StructuringElementType::RadiusType
    StructuringElementRadiusType;

  /** Set the pointer to the kernel distance metric to be used. This kernel
   * distance should be a RBF kernel distance metric. */
  virtual void SetKernelDistanceMetric( KernelDistanceMetricPointer
                                        kernelDistanceMetricPtr );

  /** Set the structuring element to be used in the shaped neighborhood
   * iterator. */
  void SetStructuringElement(StructuringElementType &structuringEl);

protected:

  /** Constructor. */
  KFCMSClassifierInitializationImageFilter();

  /** Destructor. */
  virtual ~KFCMSClassifierInitializationImageFilter() {}

  /** Write the name-value pairs of the class data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, Indent indent) const override;

  /** Check if the structuring element and the kernel function are
   * initialized.*/
  virtual void Initialize() override;

  /** KFCMSClassifierInitializationImageFilter is implemented as a
   * multithreaded filter and needs to perform processing after the output
   * image has been allocated but before threads are spawned. Therefore, this
   * implementation provides a BeforeThreadedGenerateData() method. This
   * method is executed once per iteration of the KFCMS algorithm and will
   * store the old values of the centroids and initialize some variables used
   * internally. */
  void BeforeThreadedGenerateData() override;

  /** KFCMSClassifierInitializationImageFilter is implemented as a
   * multithreaded filter. Therefore, this implementation provides a
   * ThreadedGenerateData() method which is called for each processing thread.
   * The output image data is allocated automatically by the supercalss prior
   * to calling ThreadedGenerateData(). ThreadedGenerateData() can only write
   * to the portion of the output membership image specified by the parameter
   * "outputRegionForThread". This method is executed once per iteration of the
   * KFCMS algorithm and will set that portion of the membership image and
   * perform some calulations used later to update the centroids.
   *
   * \sa FuzzyClassifierInitializationImageFilter::GenerateData() */
  /** Support for ITK 3.20 */
#if ITK_VERSION_MAJOR < 4
  typedef int ThreadIdType;
#endif
  void
    ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                             ThreadIdType threadId ) override;

  /** KFCMSClassifierInitializationImageFilter is implemented as a
   * multithreaded filter and needs to perform processing after all processing
   * threads have completed. Therefore, this implementation provides an
   * AfterThreadedGenerateData() method. This method is executed once per
   * iteration of the KFCMS algorithm and will update the centroids and
   * calculate the error. */
  void AfterThreadedGenerateData() override;


  /** Penalty factor of the neighborhood used in the membership and
   * centroid expressions. */
  double m_Alpha;

  /** Pointer to the kernel distance metric to be used. */
  KernelDistanceMetricPointer m_KernelDistanceMetric;

  /** Mutex lock used to protect the modification of attributes wich are
   * accessed through different threads. */

#if ITK_VERSION_MAJOR < 5
  MutexLockType::Pointer m_CentroidsModificationAttributesLock;
#else
  MutexLockType m_CentroidsModificationAttributesLock;
#endif

  /** Structuring element of the shaped neighborhood iterator*/
  StructuringElementType m_StructuringElement;

private:

  /** Copy constructor. Purposely not implemented. */
  KFCMSClassifierInitializationImageFilter(const Self&);

  /** Basic assignment operator. Purposely not implemented. */
  void operator=(const Self&);


  /** Array of class centroids used to store the numerator of centroids
   * equation. */
  CentroidArrayType m_CentroidsNumerator;

  /** Array used to store the denominator of centroids equation. */
  Array< CentroidValueType > m_CentroidsDenominator;

  /** Array of class centroids used to store the values of the centroids in
   * the previous iteration. */
  CentroidArrayType m_OldCentroids;

};

} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkKFCMSClassifierInitializationImageFilter.txx"
#endif

#endif // __itkKFCMSClassifierInitializationImageFilter_h

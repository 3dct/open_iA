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
#ifndef __itkFCMClassifierInitializationImageFilter_h
#define __itkFCMClassifierInitializationImageFilter_h

#include "itkFuzzyClassifierInitializationImageFilter.h"

#include "itkNumericTraits.h"
#include "itkArray.h"
#include "itkMacro.h"


#include "itkEuclideanDistanceMetric.h"


#if ITK_VERSION_MAJOR >= 5
#include <mutex>
#else
#include "itkFastMutexLock.h"
#endif


namespace itk
{

/** \class FCMClassifierInitializationImageFilter
 *
 * \brief Performs Fuzzy C-Means (FCM) Classification on an image.
 *
 * This implementation is based on Bezdek et al.'s paper "FCM: The fuzzy
 * c-means clustering algorithm" (Computers & Geosciences, 10 (2), 191-203.,
 * 1984).
 *
 * FCMClassifierInitializationImageFilter is implemented as a multithreaded
 * filter. This implementation provides BeforeThreadedGenerateData(),
 * ThreadedGenerateData() and AfterThreadedGenerateData() methods.
 * Documentation of the superclass (FuzzyClassifierInitializationImageFilter)
 * explains multhithreaded support in fuzzy classifiers.
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa FuzzyClassifierInitializationImageFilter
 * \sa KFCMSClassifierInitializationImageFilter
 * \sa MSKFCMClassifierInitializationImageFilter
 * \sa MSFKCMClassifierInitializationImageFilter
 * \sa FuzzyClassifierImageFilter
 *
 * \ingroup ClassificationFilters
 */
template< class TInputImage, class TProbabilityPrecision = double,
          class TCentroidValuePrecision = double >
class ITK_EXPORT FCMClassifierInitializationImageFilter :
    public FuzzyClassifierInitializationImageFilter< TInputImage,
                                                     TProbabilityPrecision,
                                                     TCentroidValuePrecision >
{

public:

  /** Standard class typedefs. */
  typedef FCMClassifierInitializationImageFilter    Self;
  typedef FuzzyClassifierInitializationImageFilter<
              TInputImage,
              TProbabilityPrecision,
              TCentroidValuePrecision >             Superclass;
  typedef SmartPointer< Self >                      Pointer;
  typedef SmartPointer< const Self >                ConstPointer;

  /** Input image typedef support (types inherited from the superclass). */
  typedef typename Superclass::InputImageType       InputImageType;
  typedef typename Superclass::InputImagePixelType  InputImagePixelType;
  typedef typename Superclass::InputImageRegionType InputImageRegionType;

  /** Centroid typedefs (types inherited from the superclass). */
  typedef typename Superclass::CentroidType      CentroidType;
  typedef typename Superclass::CentroidValueType CentroidValueType;
  typedef typename Superclass::CentroidArrayType CentroidArrayType;

  /** Other centroid typedefs. */
  typedef NumericTraits< CentroidType >       CentroidNumericTraitsType;
  typedef NumericTraits< CentroidValueType >  CentroidValueNumericTraitsType;

  /** Internal image typedef support (types inherited from the superclass).
   * This image is used for internal processing. */
  typedef typename Superclass::InternalImageType          InternalImageType;
  typedef typename Superclass::InternalImagePointer       InternalImagePointer;
  typedef typename Superclass::InternalImageConstPointer  InternalImageConstPointer;
  typedef typename Superclass::InternalImageConstIterator InternalImageConstIterator;

  /** Membership matrix typedef support (types inherited from the
   * superclass). */
  typedef typename Superclass::MembershipImageType      MembershipImageType;
  typedef typename Superclass::MembershipValueType      MembershipValueType;
  typedef typename Superclass::MembershipImagePixelType MembershipImagePixelType;
  typedef typename Superclass::MembershipImagePointer   MembershipImagePointer;
  typedef typename Superclass::MembershipImageIterator  MembershipImageIterator;

  /** Other membership typedefs. */
  typedef typename MembershipImageType::RegionType OutputImageRegionType;
  typedef NumericTraits< MembershipValueType >
    MembershipValueNumericTraitsType;

  /** Distance metric measure typdefs. */
#if ITK_VERSION_MAJOR < 4
#ifdef ITK_USE_REVIEW_STATISTICS
  typedef Statistics::EuclideanDistanceMetric< CentroidType >
    DistanceMetricType;
#else
  typedef Statistics::EuclideanDistance< CentroidType > DistanceMetricType;
#endif
#else
  typedef Statistics::EuclideanDistanceMetric< CentroidType >
    DistanceMetricType;
#endif
  typedef typename DistanceMetricType::Pointer DistanceMetricPointer;

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
  itkTypeMacro( FCMClassifierInitializationImageFilter,
                FuzzyClassifierInitializationImageFilter );


  /** Set number of classes. This method calls
   * Superclass::SetNumberOfClasses(numOfClasses) and creates some variables
   * used for centroids calculations. This method also sets the size of the
   * vector of old centroids.*/
  void SetNumberOfClasses(const unsigned int &numOfClasses) override;


protected:

  /** Constructor. */
  FCMClassifierInitializationImageFilter();

  /** Destructor. */
  virtual ~FCMClassifierInitializationImageFilter() {}

  /** Write the name-value pairs of the class data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, Indent indent) const override;


  /** FCMClassifierInitializationImageFilter is implemented as a multithreaded
   * filter and needs to perform processing after the output image has been
   * allocated but before threads are spawned. Therefore, this implementation
   * provides a BeforeThreadedGenerateData() method. This method is executed
   * once per iteration of the FCM algorithm and will store the old values of
   * the centroids and initialize some variables used internally. */
  void BeforeThreadedGenerateData() override;

  /** FCMClassifierInitializationImageFilter is implemented as a multithreaded
   * filter. Therefore, this implementation provides a ThreadedGenerateData()
   * method which is called for each processing thread. The output image data
   * is allocated automatically by the supercalss prior to calling
   * ThreadedGenerateData(). ThreadedGenerateData() can only write to the
   * portion of the output membership image specified by the parameter
   * "outputRegionForThread". This method is executed once per iteration of
   * the FCM algorithm and will set that portion of the membership image and
   * perform some calulations used later to update the centroids.
   *
   * \sa FuzzyClassifierInitializationImageFilter::GenerateData() */
  /** Support for ITK 3.20 */
#if ITK_VERSION_MAJOR < 4
  typedef int ThreadIdType;
#endif
  void ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                             ThreadIdType threadId ) override;

  /** FCMClassifierInitializationImageFilter is implemented as a multithreaded
   * filter and needs to perform processing after all processing threads have
   * completed. Therefore, this implementation provides an
   * AfterThreadedGenerateData() method. This method is executed once per
   * iteration of the FCM algorithm and will update the centroids and calculate
   * the error. */
  void AfterThreadedGenerateData() override;


  /** Pointer to the distance metric to be used. */
  DistanceMetricPointer m_DistanceMetric;

  /** Mutex lock used to protect the modification of attributes wich are
   * accessed through different threads. */

#if ITK_VERSION_MAJOR >= 5
  MutexLockType m_CentroidsModificationAttributesLock;
#else
  MutexLockType::Pointer m_CentroidsModificationAttributesLock;
#endif


private:

  /** Copy constructor. Purposely not implemented. */
  FCMClassifierInitializationImageFilter(const Self&);

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
#include "itkFCMClassifierInitializationImageFilter.txx"
#endif

#endif // __itkFCMClassifierInitializationImageFilter_h

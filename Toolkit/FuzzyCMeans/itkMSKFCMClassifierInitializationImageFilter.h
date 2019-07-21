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
#ifndef __itkMSKFCMClassifierInitializationImageFilter_h
#define __itkMSKFCMClassifierInitializationImageFilter_h

#include "itkFuzzyClassifierInitializationImageFilter.h"
#include "itkRBFKernelInducedDistanceMetric.h"
#include "itkFlatStructuringElement.h"

#include <itkMacro.h>
#include <itkVector.h>
#include <itkArray.h>
#include <itkConstShapedNeighborhoodIterator.h>
#include <itkNumericTraits.h>
#if ITK_VERSION_MAJOR >= 5
#include <itkMultiThreaderBase.h>
#include <mutex>
#else
#include <itkFastMutexLock.h>
#include <itkBarrier.h>
#include <itkMultiThreader.h>
#endif
#include <vector>

namespace itk
{

/** \class MSKFCMClassifierInitializationImageFilter
 *
 * \brief Performs Modified Spatial Kernelized Fuzzy C-Means (MSKFCM)
 * Classification on an image.
 *
 * This implementation is a modified version of the algorithm MSFKCM proposed
 * by Castro et al. in the paper "Comparison of various fuzzy clustering
 * algorithms in the detection of ROI in lung CT and a modified
 * kernelized-spatial fuzzy c-means algorithm" (Proc. of 10th IEEE Int. Conf.
 * On Inf. Tech. and Appl. in Biom., Corfu, Greece, 2010). This modification
 * consists in the combination of the algorithms SFCM (as in the MSFKCM
 * version) and the KFCM (instead of the KFCM used in the MSFKCM version).
 * The KFCM algorithm was presented in S. C. Chen et al.'s paper "Robust Image
 * Segmentation Using FCM With Spatial Constraints Based on New Kernel-Induced
 * Distance Measure" (IEEE Transactions on Systems, Man, and Cybernetics -
 * Part B: Cybernetics, vol. 34, no. 4, August 2004)
 *
 * MSKFCMClassifierInitializationImageFilter is implemented as a multithreaded
 * filter. This implementation provides BeforeThreadedGenerateData(),
 * ThreadedGenerateData() and AfterThreadedGenerateData() methods.
 * Documentation of the superclass (FuzzyClassifierInitializationImageFilter)
 * explains multhithreaded support in fuzzy classifiers.
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
 * \sa KFCMSClassifierInitializationImageFilter
 * \sa MSFKCMClassifierInitializationImageFilter
 * \sa FuzzyClassifierImageFilter
 *
 * \ingroup ClassificationFilters
 */
template< class TInputImage, class TProbabilityPrecision = double,
          class TCentroidValuePrecision = double >
class ITK_EXPORT MSKFCMClassifierInitializationImageFilter :
    public FuzzyClassifierInitializationImageFilter< TInputImage,
                                                     TProbabilityPrecision,
                                                     TCentroidValuePrecision >
{

public:

  /** Standard class typedefs. */
  typedef MSKFCMClassifierInitializationImageFilter Self;
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
  typedef typename Superclass::InternalImageConstIterator
                                                    InternalImageConstIterator;

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
  typedef ConstShapedNeighborhoodIterator< MembershipImageType >
    MembershipImageConstShapedNeighborhoodIterator;

  /** Type definitions for kernel distance metric measure to be used. */
  typedef Statistics::KernelInducedDistanceMetric< CentroidType >
    KernelDistanceMetricType;
  typedef typename KernelDistanceMetricType::Pointer
    KernelDistanceMetricPointer;

  /** Neighborhood region radius typedef. */
  typedef typename itk::Vector< unsigned int,
    itkGetStaticConstMacro(InputImageDimension) > NeighborhoodRadiusType;

#if ITK_VERSION_MAJOR >= 5
  typedef std::mutex MutexLockType;
#else
  /** Type definitions for mutex lock. Mutex lock allows the locking of
   * variables which are accessed through different threads. */
  typedef itk::FastMutexLock MutexLockType;

  /** Type definitions for barrier class used to synchronize threaded
   * execution. */
  typedef itk::Barrier BarrierType;
#endif


  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro( MSKFCMClassifierInitializationImageFilter,
                FuzzyClassifierInitializationImageFilter );


  /** Set number of classes. This method calls
   * Superclass::SetNumberOfClasses(numOfClasses) and creates some variables
   * used for centroids calculations. This method also sets the size of the
   * vector of old centroids and the length of the vector of the temporary
   * membership matrix.*/
  void SetNumberOfClasses(const unsigned int &numOfClasses) override;

  /** Get the exponent p that controls the relevance of the memberships
   * in the second step of the algorithm (when the spatial information is
   * introduced as in the SFCM). */
  itkGetConstMacro(P, double);

  /** Set the exponent p that controls the relevance of the memberships
   * in the second step of the algorithm (when the spatial information is
   * introduced as in the SFCM). */
  itkSetMacro(P, double);

  /** Get the exponent q that controls the relevance of the function h in the
   * second step of the algorithm (when the spatial information is introduced
   * as in the SFCM). */
  itkGetConstMacro(Q, double);

  /** Set the exponent q that controls the relevance of the function h in the
   * second step of the algorithm (when the spatial information is introduced
   * as in the SFCM). */
  itkSetMacro(Q, double);

  /** Structuring element typedef. */
  typedef typename itk::FlatStructuringElement<
  itkGetStaticConstMacro(InputImageDimension) > StructuringElementType;

  typedef typename StructuringElementType::RadiusType StructuringElementRadiusType;

  /** Set the pointer to the kernel distance metric to be used. This kernel
   * distance should be a RBF kernel distance metric. */
  virtual void SetKernelDistanceMetric( KernelDistanceMetricPointer
                                        kernelDistanceMetricPtr );

  /** Set the input image of this process object. This method calls
   * Superclass::SetInput() and allocates the temporary membership image. */
  void SetInput(const InputImageType *input) override;


  /** Set the structuring element to be used in the shaped neighborhood iterator. */
  void SetStructuringElement( StructuringElementType &
                                  structuringEl );

protected:

  /** Constructor. */
  MSKFCMClassifierInitializationImageFilter();

  /** Destructor. */
  virtual ~MSKFCMClassifierInitializationImageFilter();

  /** Write the name-value pairs of the class data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, Indent indent) const override;


  /** Initialize the barrier used for synchronizing the execution of
   * threads. */
  void Initialize() override;

  /** MSKFCMClassifierInitializationImageFilter is implemented as a
   * multithreaded filter and needs to perform processing after the output
   * image has been allocated but before threads are spawned. Therefore, this
   * implementation provides a BeforeThreadedGenerateData() method. This
   * method is executed once per iteration of the MSKFCM algorithm and will
   * store the old values of the centroids and initialize some variables used
   * internally. */
  void BeforeThreadedGenerateData() override;

  /** MSKFCMClassifierInitializationImageFilter is implemented as a
   * multithreaded filter. Therefore, this implementation provides a
   * ThreadedGenerateData() method which is called for each processing thread.
   * The output image data is allocated automatically by the supercalss prior
   * to calling ThreadedGenerateData(). ThreadedGenerateData() can only write
   * to the portion of the output membership image specified by the parameter
   * "outputRegionForThread". This method is executed once per iteration of the
   * MSKFCM algorithm and will set that portion of the membership image and
   * perform some calulations used later to update the centroids.
   *
   * \sa FuzzyClassifierInitializationImageFilter::GenerateData() */
#if ITK_VERSION_MAJOR < 4
  typedef int ThreadIdType;
#endif
  void ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                             ThreadIdType threadId ) override;

  /** MSKFCMClassifierInitializationImageFilter is implemented as a
   * multithreaded filter and needs to perform processing after all processing
   * threads have completed. Therefore, this implementation provides an
   * AfterThreadedGenerateData() method. This method is executed once per
   * iteration of the MSKFCM algorithm and will update the centroids and
   * calculate the error. */
  void AfterThreadedGenerateData() override;


  /** Exponent that controls the relevance of the memberships in the second
   * step of the algorithm (when the spatial information is introduced as in
   * the SFCM). */
  double m_P;

  /** Exponent that controls the relevance of the function h in the second
   * step of the algorithm (when the spatial information is introduced as in
   * the SFCM). */
  double m_Q;

  /** Pointer to the kernel distance metric to be used. */
  KernelDistanceMetricPointer m_KernelDistanceMetric;

#if ITK_VERSION_MAJOR < 5
  /** Mutex lock used to protect the modification of attributes wich are
   * accessed through different threads. */
  MutexLockType::Pointer m_CentroidsModificationAttributesLock;

  /** Standard barrier for synchronizing the execution of threads. */
  BarrierType::Pointer m_Barrier;
#else
  MutexLockType m_CentroidsModificationAttributesLock;
#endif

  /** Structuring element of the shaped neighborhood iterator*/
  StructuringElementType m_StructuringElement;

private:

  /** Copy constructor. Purposely not implemented. */
  MSKFCMClassifierInitializationImageFilter(const Self&);

  /** Basic assignment operator. Purposely not implemented. */
  void operator=(const Self&);


  /** Array of class centroids used to store the numerator of centroids
   * equation (KFCM). */
  CentroidArrayType m_CentroidsNumerator;

  /** Array used to store the denominator of centroids equation (KFCM). */
  Array< CentroidValueType > m_CentroidsDenominator;

  /** Array of class centroids used to store the values of the centroids in
   * the previous iteration. */
  CentroidArrayType m_OldCentroids;

  /** Temporary membership image used to store the values of the memberships
   * on the first step of the algorithm (KFCM). */
  MembershipImagePointer m_TmpMembershipImage;

};

} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMSKFCMClassifierInitializationImageFilter.txx"
#endif

#endif // __itkMSKFCMClassifierInitializationImageFilter_h

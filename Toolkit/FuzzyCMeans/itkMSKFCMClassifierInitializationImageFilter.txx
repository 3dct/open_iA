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
#ifndef __itkMSKFCMClassifierInitializationImageFilter_txx
#define __itkMSKFCMClassifierInitializationImageFilter_txx

#include "itkMSKFCMClassifierInitializationImageFilter.h"

namespace itk
{

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::MSKFCMClassifierInitializationImageFilter()
{
  m_P = 1;
  m_Q = 1;

  typedef Statistics::RBFKernelInducedDistanceMetric< CentroidType >
    RBFKernelDistanceMetricType;
  typedef typename RBFKernelDistanceMetricType::Pointer RBFKernelDistanceMetricPointer;
  RBFKernelDistanceMetricPointer kernelDistance = RBFKernelDistanceMetricType::New();
  kernelDistance->SetA(2.0);
  kernelDistance->SetB(1.0);
  kernelDistance->SetSigma(300);
  m_KernelDistanceMetric = kernelDistance;

  typename StructuringElementType::RadiusType elementRadius;
  elementRadius.Fill(1);
  m_StructuringElement = StructuringElementType::Box(elementRadius);

  m_CentroidsModificationAttributesLock = MutexLockType::New();

  m_TmpMembershipImage = MembershipImageType::New();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::~MSKFCMClassifierInitializationImageFilter()
{}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "P : " << m_P << std::endl;
  os << indent << "Q : " << m_Q << std::endl;
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::SetInput(const InputImageType *input)
{
  Superclass::SetInput(input);
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::SetNumberOfClasses(const unsigned int &numOfClasses)
{
  Superclass::SetNumberOfClasses(numOfClasses);

  m_OldCentroids = CentroidArrayType(this->m_NumberOfClasses);

  // These variables are used to accumulate the numerator and the denominator
  // of centroid expression.
  m_CentroidsNumerator = CentroidArrayType(this->m_NumberOfClasses);
  m_CentroidsDenominator.set_size(this->m_NumberOfClasses);

  m_TmpMembershipImage->SetVectorLength(this->m_NumberOfClasses);
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::SetKernelDistanceMetric(KernelDistanceMetricPointer kernelDistanceMetricPtr)
{
  itkDebugMacro("Setting m_KernelDistanceMetric");

  if (kernelDistanceMetricPtr.IsNull())
    {
    itkExceptionMacro("Pointer to Kernel Distance Metric is Null")
    }

  m_KernelDistanceMetric = kernelDistanceMetricPtr;
  this->Modified();
}

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::SetStructuringElement(StructuringElementType& structuringEl)
{
  itkDebugMacro("Setting m_StructuringElement");

  m_StructuringElement = structuringEl;
  this->Modified();
}

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::Initialize()
{
  itkDebugMacro("Computing initializations before start the iterations");

  if (m_StructuringElement.Size() == 0)
    {
    itkGenericExceptionMacro(<< "Structuring Element not assigned ");
    }

  if (m_KernelDistanceMetric.IsNull())
    {
    itkGenericExceptionMacro(<< "Kernel function not assigned ");
    }

  m_TmpMembershipImage->SetRegions(this->GetInput()->GetLargestPossibleRegion());
  m_TmpMembershipImage->Allocate();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::BeforeThreadedGenerateData()
{
  itkDebugMacro("Starting BeforeThreadedGenerateData()");

  unsigned int i;

  // Store the old values of the centroids and initialize the numerators and
  // denominators of centroid expression.
  for (i = 0; i < this->m_NumberOfClasses; i++)
    {
    m_OldCentroids[i] = this->m_Centroids[i];

    m_CentroidsNumerator[i] = CentroidNumericTraitsType::Zero;
    }
  m_CentroidsDenominator.Fill(CentroidValueNumericTraitsType::Zero);

  ThreadIdType numThreads = this->GetNumberOfThreads();
  if( itk::MultiThreader::GetGlobalMaximumNumberOfThreads() != 0 )
	{
	numThreads =  std::min( this->GetNumberOfThreads(),
        itk::MultiThreader::GetGlobalMaximumNumberOfThreads() );
    }
  // number of threads can be constrained by the region size, so call the
  //SplitRequestedRegion to get the real number of threads which will be used
  OutputImageRegionType splitRegion;  // dummy region - just to call the following method
  numThreads = this->SplitRequestedRegion(0, numThreads, splitRegion);

  m_Barrier = Barrier::New();
  m_Barrier->Initialize( numThreads );
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                        ThreadIdType itkNotUsed(threadId) )
{
  itkDebugMacro("Starting ThreadedGenerateData()");

  unsigned int i;

  double exponentOfMembership = - (1.0 / (this->m_M - 1.0));

  MembershipImagePixelType tmpKernel(this->m_NumberOfClasses);

  MembershipImagePixelType tmpMembershipNumerator(this->m_NumberOfClasses);
  double tmpMembershipDenominator;

  // If background is ignored then background pixel is computed as centroid
  // type and their memberships are set to -1. Background memberships are used
  // for background values in the resulting membership image.
  CentroidType backgroundPixelAsCentroid;
  MembershipImagePixelType backgroundMembershipPixel;
  if (this->m_IgnoreBackgroundPixels)
    {
    this->ComputePixelAsCentroid( this->m_BackgroundPixel,
                                  backgroundPixelAsCentroid );
    backgroundMembershipPixel.SetSize(this->m_NumberOfClasses);
    backgroundMembershipPixel.Fill(-1);
    }

  MembershipImagePixelType tmpMembershipPixel(this->m_NumberOfClasses);

  InternalImageConstIterator itrImageToProcess( this->m_ImageToProcess,
                                                outputRegionForThread );
  MembershipImageIterator itrTmpMembershipMatrix( m_TmpMembershipImage,
                                                  outputRegionForThread );
  MembershipImageIterator itrMembershipMatrix( this->GetOutput(),
                                               outputRegionForThread );

  itkDebugMacro("Starting KFCM calculations");

  // Compute the initial degree of membership, using the same equation as the
  // KFCM.
  for( itrImageToProcess.GoToBegin(), itrTmpMembershipMatrix.GoToBegin(),
       itrMembershipMatrix.GoToBegin();
       !itrImageToProcess.IsAtEnd();
       ++itrImageToProcess, ++itrTmpMembershipMatrix, ++itrMembershipMatrix )
    {
    // If the algorithm should ignore the background of the image, pixels with
    // this value does not belong to any class. Thus their memberships and
    // their kernel values are set to -1.
    if ( this->m_IgnoreBackgroundPixels &&
         (itrImageToProcess.Get() == backgroundPixelAsCentroid) )
      {
      itrTmpMembershipMatrix.Set(backgroundMembershipPixel);
      itrMembershipMatrix.Set(backgroundMembershipPixel);
      continue;
      }

    tmpMembershipDenominator = 0.0;

    // Calculate the numerator and denominator of the memberships.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      tmpKernel[i] = m_KernelDistanceMetric->Evaluate( itrImageToProcess.Get(),
                                                       this->m_Centroids[i] );

      tmpMembershipNumerator[i] =
        std::pow((1.0 - tmpKernel[i]), exponentOfMembership);

      tmpMembershipDenominator += tmpMembershipNumerator[i];
      }

    // Save kernel distance between current pixel and the centroids in the
    // membership matrix. This values are used later in the centroids
    // equation.
    itrMembershipMatrix.Set(tmpKernel);

    // Calculate the memberships as in KFCM.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      if ( ( tmpMembershipNumerator[i] ==
             MembershipValueNumericTraitsType::infinity() ) &&
           ( tmpMembershipDenominator ==
             NumericTraits< double >::infinity() ) )
        {
        tmpMembershipPixel[i] = 1.0;
        }
      else
        {
        tmpMembershipPixel[i] = tmpMembershipNumerator[i] /
                                tmpMembershipDenominator;
        }
      }

    // Save first pixel memberships in a temporary membership matrix.
    itrTmpMembershipMatrix.Set(tmpMembershipPixel);
    }

  // Synchronize threaded execution. As each thread enters the barrier it
  // blocks. When all threads have entered the barrier (and therefore all
  // memberships required to compute the second step of the procedure have
  // been calculated), all released and continue to execute.
  this->m_Barrier->Wait();

  // These variables are used to accumulate the numerator and the denominator
  // of centroid expression.
  CentroidArrayType
    tempThreadCentroidsNumerator( this->m_NumberOfClasses,
                                  CentroidNumericTraitsType::Zero );
  Array< CentroidValueType >
    tempThreadCentroidsDenominator(this->m_NumberOfClasses);
  tempThreadCentroidsDenominator.Fill(CentroidValueNumericTraitsType::Zero);

  typename StructuringElementType::ConstIterator nit;
  StructuringElementRadiusType radiusStructEl;

  typename StructuringElementType::NeighborIndexType idx;

  idx =0;
  radiusStructEl = m_StructuringElement.GetRadius();

  MembershipImageConstShapedNeighborhoodIterator
     itrNeighborhoodTmpMembershipMatrix( m_StructuringElement.GetRadius(), m_TmpMembershipImage, outputRegionForThread );

  bool isInBounds;

  if (itrNeighborhoodTmpMembershipMatrix.GetRadius() != radiusStructEl)
    {
   itkGenericExceptionMacro(<< "Radius of shaped iterator("
                             << itrNeighborhoodTmpMembershipMatrix.GetRadius()
                             << ") does not equal radius of neighborhood("
                             << radiusStructEl << ")");
    }

  for (nit = m_StructuringElement.Begin(); nit != m_StructuringElement.End(); ++nit,++idx)
    {
    if (*nit)
      {
      itrNeighborhoodTmpMembershipMatrix.ActivateOffset(itrNeighborhoodTmpMembershipMatrix.GetOffset(idx));
      }
    else
      {
      itrNeighborhoodTmpMembershipMatrix.DeactivateOffset(itrNeighborhoodTmpMembershipMatrix.GetOffset(idx));
      }
    }

  MembershipImagePixelType currentNeighborPixel(this->m_NumberOfClasses);

  MembershipImagePixelType membershipPixel(this->m_NumberOfClasses);

  Array< double > h(this->m_NumberOfClasses);

  MembershipImagePixelType membershipNumerator(this->m_NumberOfClasses);
  double membershipDenominator;

  MembershipImagePixelType membershipPixelPowM(this->m_NumberOfClasses);

  itkDebugMacro("Starting SFCM steps and overlap some KFCM calculations");

  // The second step introduces the spatial information as in the SFCM
  // procedure and calculates the new memberships. In addition, some
  // calculations for the centroids (KFCM) overlap.
  for( itrMembershipMatrix.GoToBegin(), itrImageToProcess.GoToBegin(),
       itrNeighborhoodTmpMembershipMatrix.GoToBegin();
       !itrMembershipMatrix.IsAtEnd();
       ++itrMembershipMatrix, ++itrImageToProcess,
       ++itrNeighborhoodTmpMembershipMatrix )
    {
    // If the algorithm should ignore the background of the image, pixels with
    // this value does not belong to any class. Thus their memberships are set
    // to -1.
    if ( this->m_IgnoreBackgroundPixels &&
         (itrImageToProcess.Get() == backgroundPixelAsCentroid) )
      {
      itrMembershipMatrix.Set(backgroundMembershipPixel);
      continue;
      }

    typename MembershipImageConstShapedNeighborhoodIterator::ConstIterator
      itrNeighborhood = itrNeighborhoodTmpMembershipMatrix.Begin();

    h.Fill(0.0);

    // The spatial intormation is introduced by the function h.
    for ( itrNeighborhood.GoToBegin(); !itrNeighborhood.IsAtEnd();
          ++itrNeighborhood )
      {
      currentNeighborPixel =
        itrNeighborhoodTmpMembershipMatrix.GetPixel(
          itrNeighborhood.GetNeighborhoodOffset(), isInBounds );

      // If the offset is outside the image and the pixel value returned is a
      // boundary condition, the neighbor is ignored (i.e. the algorithm only
      // processes actual pixels).
      if (!isInBounds) { continue; }

      // If the algorithm should ignore the background of the image, neighbors
      // with this value are ignored.
      if ( this->m_IgnoreBackgroundPixels &&
           (itrNeighborhood.Get() == backgroundMembershipPixel) )
        {
        continue;
        }

      for (i = 0; i < this->m_NumberOfClasses; i++)
        {
        h[i] += itrNeighborhood.Get()[i];
        }
      }

    membershipDenominator = 0.0;

    // Calculate the numerator and denominator of the memberships.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      membershipNumerator[i] =
        std::pow( itrNeighborhoodTmpMembershipMatrix.GetCenterPixel()[i],
                 this->m_P ) *
		std::pow(h[i], this->m_Q);
      membershipDenominator += membershipNumerator[i];
      }

    // Calculate the new memberships as in SFCM. In addition, some
    // calculations for the centroids (KFCM) overlap.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      membershipPixel[i] = membershipNumerator[i] / membershipDenominator;

      // Calculations for the centroids.
      membershipPixelPowM[i] = std::pow(membershipPixel[i], this->m_M);
      tempThreadCentroidsNumerator[i] +=
        itrImageToProcess.Get() * (itrMembershipMatrix.Get()[i]) *
        membershipPixelPowM[i];
      tempThreadCentroidsDenominator[i] +=
        membershipPixelPowM[i] * (itrMembershipMatrix.Get()[i]);
      }

    // Save new pixel memberships in the membership matrix (and therefore
    // delete the kernel distance between current pixel and the centroids
    // stored previously).
    itrMembershipMatrix.Set(membershipPixel);
    }

  m_CentroidsModificationAttributesLock->Lock();

  for (i = 0; i < this->m_NumberOfClasses; i++)
    {
    m_CentroidsNumerator[i] += tempThreadCentroidsNumerator[i];
    m_CentroidsDenominator[i] += tempThreadCentroidsDenominator[i];
    }

  m_CentroidsModificationAttributesLock->Unlock();

}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
MSKFCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::AfterThreadedGenerateData()
{
  itkDebugMacro("Starting AfterThreadedGenerateData()");

  unsigned int i;

  // Calculate the new centroid values.
  for (i = 0; i < this->m_NumberOfClasses; i++)
    {
    this->m_Centroids[i] = m_CentroidsNumerator[i] / m_CentroidsDenominator[i];
    }
  this->Modified();

  // Calculate current error.
  this->m_Error = this->ComputeDifference(m_OldCentroids);
  this->Modified();
}

} // end of namespace itk

#endif // __itkMSKFCMClassifierInitializationImageFilter_txx

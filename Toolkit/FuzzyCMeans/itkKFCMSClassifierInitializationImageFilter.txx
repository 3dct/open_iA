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
#ifndef __itkKFCMSClassifierInitializationImageFilter_txx
#define __itkKFCMSClassifierInitializationImageFilter_txx

#include "itkKFCMSClassifierInitializationImageFilter.h"

namespace itk
{

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::KFCMSClassifierInitializationImageFilter()
{
  m_Alpha = 0.5;

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
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Alpha : " << m_Alpha << std::endl;
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::SetNumberOfClasses(const unsigned int &numOfClasses)
{
  Superclass::SetNumberOfClasses(numOfClasses);

  m_OldCentroids = CentroidArrayType(this->m_NumberOfClasses);

  // These variables are used to accumulate the numerator and the denominator
  // of centroid expression.
  m_CentroidsNumerator = CentroidArrayType(this->m_NumberOfClasses);
  m_CentroidsDenominator.set_size(this->m_NumberOfClasses);
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                           TCentroidValuePrecision >
::Initialize()
{
   if(m_StructuringElement.Size() == 0)
     {
     itkGenericExceptionMacro(<< "Structuring Element not assigned ");
     }

   if(m_KernelDistanceMetric.IsNull())
     {
     itkGenericExceptionMacro(<< "Kernel function not assigned ");
     }
}

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                        ThreadIdType threadId )
{
  itkDebugMacro("Starting ThreadedGenerateData()");

  unsigned int i;

  // This variable represents the number of neighbors of a pixel that are not
  // ignored (i.e. they are not background pixels).
  unsigned int numberOfNeighbors;

  double currentNeighborDistance;
  Array< double > currentPixelDistance(this->m_NumberOfClasses);
  double penaltyFactor;
  MembershipImagePixelType
    tmpNeighborhoodFactorOfMemberships(this->m_NumberOfClasses);
  CentroidArrayType
    tmpNeighborhoodFactorOfCentroidsNumerator(this->m_NumberOfClasses);
  Array< CentroidValueType >
    tmpNeighborhoodFactorOfCentroidsDenominator(this->m_NumberOfClasses);
  double tmpPowMembershipValue;
  double exponentOfMembership = - (1.0 / (this->m_M - 1.0));

  MembershipImagePixelType membershipNumerator(this->m_NumberOfClasses);
  MembershipValueType membershipDenominator;

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

#if ITK_VERSION_MAJOR > 3
  typename StructuringElementType::NeighborIndexType idx;
#else
  unsigned int idx;
#endif

  idx =0;
  radiusStructEl = m_StructuringElement.GetRadius();

  InternalImageConstShapedNeighborhoodIterator
    itrImageToProcess(radiusStructEl, this->m_ImageToProcess, outputRegionForThread);

  bool isInBounds;

  if (itrImageToProcess.GetRadius() != radiusStructEl)
    {
   itkGenericExceptionMacro(<< "Radius of shaped iterator("
                             << itrImageToProcess.GetRadius()
                             << ") does not equal radius of neighborhood("
                             << radiusStructEl << ")");
    }

  for (nit = m_StructuringElement.Begin(); nit != m_StructuringElement.End(); ++nit,++idx)
    {
    if (*nit)
      {
      itrImageToProcess.ActivateOffset(itrImageToProcess.GetOffset(idx));
      }
    else
      {
      itrImageToProcess.DeactivateOffset(itrImageToProcess.GetOffset(idx));
      }
    }

  // Deactivate center pixel.
  typename InternalImageConstShapedNeighborhoodIterator::OffsetType offset;
  offset.Fill(0);
  itrImageToProcess.DeactivateOffset(offset);

  MembershipImageIterator itrMembershipMatrix( this->GetOutput(),
                                               outputRegionForThread );

  CentroidType currentPixel;
  CentroidType currentNeighborPixel;

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

  MembershipImagePixelType membershipPixel(this->m_NumberOfClasses);

  // Calculate the new memberships and accumulate the numerators and
  // denominators used to calculate the new class centroids.
  for ( itrImageToProcess.GoToBegin(), itrMembershipMatrix.GoToBegin();
        !itrImageToProcess.IsAtEnd();
        ++itrImageToProcess, ++itrMembershipMatrix )
    {
    currentPixel = itrImageToProcess.GetCenterPixel();

    // If the algorithm should ignore the background of the image, pixels with
    // this value does not belong to any class. Thus their memberships are set
    // to -1.
    if ( this->m_IgnoreBackgroundPixels &&
         (currentPixel == backgroundPixelAsCentroid) )
      {
      itrMembershipMatrix.Set(backgroundMembershipPixel);
      continue;
      }

    tmpNeighborhoodFactorOfMemberships.Fill(
      MembershipValueNumericTraitsType::Zero );
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      tmpNeighborhoodFactorOfCentroidsNumerator[i] =
        CentroidNumericTraitsType::Zero;
      }
    tmpNeighborhoodFactorOfCentroidsDenominator.Fill(
      CentroidValueNumericTraitsType::Zero );

    // Iterate over the neighborhood to perform some calculations. These
    // calculations are used later in membership and centroid expressions.
    // Note: the center pixel is not used (it was previously deactivated).
    numberOfNeighbors = 0;
    typename InternalImageConstShapedNeighborhoodIterator::ConstIterator
      itrNeighborhood = itrImageToProcess.Begin();
    for ( itrNeighborhood.GoToBegin(); !itrNeighborhood.IsAtEnd();
          ++itrNeighborhood )
      {
      currentNeighborPixel =
        itrImageToProcess.GetPixel( itrNeighborhood.GetNeighborhoodOffset(),
                                    isInBounds );

      // If the offset is outside the image and the pixel value returned is a
      // boundary condition, the neighbor is ignored (i.e. the algorithm only
      // processes actual pixels).
      if (!isInBounds) { continue; }

      // If the algorithm should ignore the background of the image, neighbors
      // with this value are ignored.
      if ( this->m_IgnoreBackgroundPixels &&
           (currentNeighborPixel == backgroundPixelAsCentroid) ) { continue; }

      for (i = 0; i < this->m_NumberOfClasses; i++)
        {
        currentNeighborDistance =
          m_KernelDistanceMetric->Evaluate( currentNeighborPixel,
                                            this->m_Centroids[i] );

        tmpNeighborhoodFactorOfMemberships[i] +=
          vcl_pow(1.0 - currentNeighborDistance, this->m_M);

        tmpNeighborhoodFactorOfCentroidsNumerator[i] +=
          currentNeighborDistance * currentNeighborPixel;
        tmpNeighborhoodFactorOfCentroidsDenominator[i] +=
          currentNeighborDistance;
        }

      // The number of neighbors is increased (the neighbor is not a
      // background pixel).
      numberOfNeighbors++;
      }

    // Set the penalty factor of the neighbors.
    penaltyFactor = (numberOfNeighbors == 0) ?
                    0 : m_Alpha / ((double) numberOfNeighbors);

    membershipDenominator = MembershipValueNumericTraitsType::Zero;

    // Perform some calculations for the memberships.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      currentPixelDistance[i] =
        m_KernelDistanceMetric->Evaluate( currentPixel,
                                          this->m_Centroids[i] );

      membershipNumerator[i] =
        vcl_pow( (1 - currentPixelDistance[i]) +
                 (penaltyFactor * tmpNeighborhoodFactorOfMemberships[i]),
                 exponentOfMembership );
      membershipDenominator += membershipNumerator[i];
      }

    // Calculate the pixel memberships. In addition, some calculations for the
    // centroids overlap to avoid iterating through the image twice for each
    // iteration of the KFCMS algorithm.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      if ( ( membershipNumerator[i] ==
             MembershipValueNumericTraitsType::infinity() ) &&
           ( membershipDenominator ==
             MembershipValueNumericTraitsType::infinity() ) )
        {
        membershipPixel[i] = 1.0;
        }
      else
        {
        membershipPixel[i] = membershipNumerator[i] / membershipDenominator;
        }

      // Calculations for the centroids.
      tmpPowMembershipValue = vcl_pow(membershipPixel[i], this->m_M);
      tempThreadCentroidsNumerator[i] += tmpPowMembershipValue *
        ( (currentPixelDistance[i] * currentPixel) +
          (penaltyFactor * tmpNeighborhoodFactorOfCentroidsNumerator[i]) );
      tempThreadCentroidsDenominator[i] += tmpPowMembershipValue *
        ( currentPixelDistance[i] +
          (penaltyFactor * tmpNeighborhoodFactorOfCentroidsDenominator[i]) );
      }

    // Save new pixel memberships in the membership matrix.
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
KFCMSClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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

#endif // __itkKFCMSClassifierInitializationImageFilter_txx

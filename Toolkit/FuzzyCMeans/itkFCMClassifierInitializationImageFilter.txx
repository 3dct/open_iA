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
#ifndef __itkFCMClassifierInitializationImageFilter_txx
#define __itkFCMClassifierInitializationImageFilter_txx

#include "itkFCMClassifierInitializationImageFilter.h"

namespace itk
{

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                        TCentroidValuePrecision >
::FCMClassifierInitializationImageFilter()
{
  m_DistanceMetric = DistanceMetricType::New();
#if ITK_VERSION_MAJOR < 5
  m_CentroidsModificationAttributesLock = MutexLockType::New();
#endif
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                        TCentroidValuePrecision >
::PrintSelf(std::ostream &os, Indent indent) const

{
  Superclass::PrintSelf(os, indent);
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                        TCentroidValuePrecision >
::ThreadedGenerateData( const OutputImageRegionType& outputRegionForThread,
                        ThreadIdType itkNotUsed(threadId) )
{
  itkDebugMacro("Starting ThreadedGenerateData()");

  unsigned int i;
  unsigned int j;

  double distanceOfNumerator;
  double distanceOfDenominator;
  double exponentOfMembership = 2.0 / (this->m_M - 1.0);

  MembershipValueType tmpMembershipValue;
  MembershipValueType tmpPowMembershipValue;

  // These variables are used to accumulate the numerator and the denominator
  // of centroid expression.
  CentroidArrayType
    tempThreadCentroidsNumerator( this->m_NumberOfClasses,
                                  CentroidNumericTraitsType::Zero );
  Array< CentroidValueType >
    tempThreadCentroidsDenominator(this->m_NumberOfClasses);
  tempThreadCentroidsDenominator.Fill(CentroidValueNumericTraitsType::Zero);

  // Iterator over the internal image used by the algorithm. This image is
  // obtained by converting the pixels of the input image to the type of
  // centroids.
  InternalImageConstIterator itrImageToProcess( this->m_ImageToProcess,
                                                outputRegionForThread );
  MembershipImageIterator itrMembershipMatrix( this->GetOutput(),
                                               outputRegionForThread );

  CentroidType currentPixel;

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
    currentPixel = itrImageToProcess.Get();

    // If the algorithm should ignore the background of the image, pixels with
    // this value does not belong to any class. Thus their memberships are set
    // to -1.
    if ( this->m_IgnoreBackgroundPixels &&
         (currentPixel == backgroundPixelAsCentroid) )
      {
      itrMembershipMatrix.Set(backgroundMembershipPixel);
      continue;
      }

    // Calculate the pixel memberships. In addition, some calculations for the
    // centroids overlap to avoid iterating through the image twice for each
    // iteration of the FCM algorithm.
    for (i = 0; i < this->m_NumberOfClasses; i++)
      {
      distanceOfNumerator = m_DistanceMetric->Evaluate( this->m_Centroids[i],
                                                        currentPixel );

      tmpMembershipValue = MembershipValueNumericTraitsType::Zero;

      for (j = 0; j < this->m_NumberOfClasses; j++)
        {
        distanceOfDenominator =
          m_DistanceMetric->Evaluate( this->m_Centroids[j], currentPixel );

        if ((distanceOfNumerator == 0.0) && (distanceOfDenominator == 0.0))
          {
          tmpMembershipValue += 1.0;
          }
        else
          {
          tmpMembershipValue += std::pow( ( distanceOfNumerator /
                                           distanceOfDenominator ),
                                         exponentOfMembership );
          }
        }

      membershipPixel[i] = 1.0 / tmpMembershipValue;

      // Calculations for the centroids.
      tmpPowMembershipValue = std::pow(membershipPixel[i], this->m_M);
      tempThreadCentroidsNumerator[i] += (tmpPowMembershipValue * currentPixel);
      tempThreadCentroidsDenominator[i] += tmpPowMembershipValue;
      }

    // Save new pixel memberships in the membership matrix.
    itrMembershipMatrix.Set(membershipPixel);
    }
#if ITK_VERSION_MAJOR < 5
  m_CentroidsModificationAttributesLock->Lock();
#else
  m_CentroidsModificationAttributesLock.lock();
#endif
  for (i = 0; i < this->m_NumberOfClasses; i++)
    {
    m_CentroidsNumerator[i] += tempThreadCentroidsNumerator[i];
    m_CentroidsDenominator[i] += tempThreadCentroidsDenominator[i];
    }
#if ITK_VERSION_MAJOR < 5
  m_CentroidsModificationAttributesLock->Unlock();
#else
  m_CentroidsModificationAttributesLock.unlock();
#endif
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FCMClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
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

#endif // __itkFCMClassifierInitializationImageFilter_txx

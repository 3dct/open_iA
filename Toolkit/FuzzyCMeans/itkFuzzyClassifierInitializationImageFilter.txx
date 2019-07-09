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
#ifndef __itkFuzzyClassifierInitializationImageFilter_txx
#define __itkFuzzyClassifierInitializationImageFilter_txx

#include "itkFuzzyClassifierInitializationImageFilter.h"

namespace itk
{

template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::FuzzyClassifierInitializationImageFilter(void)
{
  m_NumberOfClasses = 0;
  m_MaximumNumberOfIterations = 50;
  m_NumberOfIterations = 0;
  m_MaximumError = 0.1;
  m_Error = NumericTraits< double >::max();
  m_M = 2.0;

  // By default all pixels are processed and the background pixel is set to
  // its minimum possible value.
  m_IgnoreBackgroundPixels = false;
  m_BackgroundPixel = InputImagePixelNumericTraitsType::min();

  m_ImageToProcess = InternalImageType::New();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfClasses: " << m_NumberOfClasses << std::endl;
  os << indent << "MaximumNumberOfIterations: " << m_MaximumNumberOfIterations
     << std::endl;
  os << indent << "MaximumError: " << m_MaximumError << std::endl;
  os << indent << "M: " << m_M << std::endl;
  os << indent << "IgnoreBackgroundPixels: "
     << (m_IgnoreBackgroundPixels ? "On" : "Off") << std::endl;
  os << indent << "BackgroundPixel: " << m_BackgroundPixel << std::endl;
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::GenerateData()
{
  itkDebugMacro("Starting GenerateData()");

  if (this->m_NumberOfClasses == 0)
    {
    itkExceptionMacro( << "Number of classes unspecified" );
    }

  // Initialize the internal image used by the algorithm.
  this->ComputeImageToProcess(m_ImageToProcess);

  // Call a method that can be overridden by a subclass to allocate
  // memory for the filter's outputs
  this->AllocateOutputs();

  // Set up the multithreaded processing
  ThreadStruct str;
  str.Filter = this;

  this->GetMultiThreader()->SetNumberOfThreads(this->GetNumberOfThreads());
  this->GetMultiThreader()->SetSingleMethod(this->ThreaderCallback, &str);

  this->Initialize();

  m_NumberOfIterations = 0;
  do
    {
    m_NumberOfIterations++;

    itkDebugMacro(<< "Iteration number " << m_NumberOfIterations);

    // Call a method that can be overridden by a subclass to perform some
    // calculations prior to splitting the main computations into separate
    // threads.
    this->BeforeThreadedGenerateData();

    // Multithread the execution of the fuzzy algorithm.
    this->GetMultiThreader()->SingleMethodExecute();

    // Call a method that can be overridden by a subclass to perform some
    // calculations after all the threads have completed.
    this->AfterThreadedGenerateData();

    // The progress is updated after each iteration of the algorithm.
    this->UpdateProgress( m_NumberOfIterations /
                          ((double) m_MaximumNumberOfIterations) );
    }
  while ( (m_Error > m_MaximumError) &&
          (m_NumberOfIterations < m_MaximumNumberOfIterations) );

  // The progress is updated to completed if the algorithm ends before
  // reaching the maximum iteration.
  if (m_NumberOfIterations < m_MaximumNumberOfIterations)
    {
    this->UpdateProgress(1.0);
    }

  // Internal image should be cleaned up by SmartPointers automatically once
  // all references to the filter instance disappear.
  //this->m_ImageToProcess->Delete();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::GenerateOutputInformation()
{
  // Call the superclass' implementation of this method.
  Superclass::GenerateOutputInformation();

  MembershipImagePointer outputPtr = this->GetOutput();
  if (outputPtr)
    {
    if (this->m_NumberOfClasses == 0)
      {
      itkExceptionMacro( << "Number of classes unspecified");
      }

    outputPtr->SetVectorLength(this->m_NumberOfClasses);
    }
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
inline void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::ComputePixelAsCentroid( const InputImagePixelType &pixel,
                          CentroidType &pixelAsCentroid )
{
  itkDebugMacro("Computing pixel as centroid");

  unsigned int i;

  vnl_vector< InputImagePixelValueType > tmp(InputImagePixelDimension);
  tmp.set( (InputImagePixelValueType *) &pixel );

  for (i = 0; i < InputImagePixelDimension; i++)
    {
    pixelAsCentroid[i] = static_cast< CentroidValueType >(tmp[i]);
    }
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::ComputeImageToProcess(InternalImageType *imageToProcess)
{
  itkDebugMacro("Computing internal image to process");

  const InputImageType *inputImage = this->GetInput();
  InputImageRegionType imageRegion = inputImage->GetBufferedRegion();
  CentroidType pixel;

  // Allocate the internal image memory.
  imageToProcess->SetRegions(imageRegion);
  imageToProcess->Allocate();

  InputImageConstIterator itrInputImage(inputImage, imageRegion);
  InternalImageIterator itrImageToProcess(imageToProcess, imageRegion);

  for ( itrInputImage.GoToBegin(), itrImageToProcess.GoToBegin();
        !itrInputImage.IsAtEnd(); ++itrInputImage, ++itrImageToProcess )
    {
    this->ComputePixelAsCentroid(itrInputImage.Get(), pixel);
    itrImageToProcess.Set(pixel);
    }
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::SetNumberOfClasses(const unsigned int &numOfClasses)
{
  itkDebugMacro("Setting m_NumberOfClasses");

  unsigned int i;

  m_NumberOfClasses = numOfClasses;
  m_Centroids = CentroidArrayType(numOfClasses);

  for (i = 0; i < m_NumberOfClasses; i++)
    {
    m_Centroids[i] = CentroidValueNumericTraitsType::Zero;
    }

  this->Modified();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
typename FuzzyClassifierInitializationImageFilter< TInputImage,
                                                   TProbabilityPrecision,
                                                   TCentroidValuePrecision >::
                                                       CentroidArrayType
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::GetCentroids() const
{
  itkDebugMacro("Returning m_Centroids");

  return m_Centroids;
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
void
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::SetCentroids(const CentroidArrayType newCentroids)
{
  itkDebugMacro("Setting m_Centroids");

  if (newCentroids.size() != m_NumberOfClasses)
    {
    itkExceptionMacro( <<
      "Centroid array length should be the same as the number of classes" );
    }

  unsigned int i;

  for (i = 0; i < m_NumberOfClasses; i++)
    {
    m_Centroids[i] = newCentroids[i];
    }

  this->Modified();
}


template< class TInputImage, class TProbabilityPrecision,
          class TCentroidValuePrecision >
double 
FuzzyClassifierInitializationImageFilter< TInputImage, TProbabilityPrecision,
                                          TCentroidValuePrecision >
::ComputeDifference(const CentroidArrayType &arrayOfCentroids)
{
  itkDebugMacro("Computing difference between centroids");

  if (arrayOfCentroids.size() != m_NumberOfClasses)
    {
    itkExceptionMacro( <<
      "Centroid array length should be the same as the number of classes" );
    }

  unsigned int i;
  unsigned int j;

  double difference = 0.0;

  for (i = 0; i < m_NumberOfClasses; i++)
    {
    for (j = 0; j < InputImagePixelDimension; j++)
      {
      difference += std::pow((m_Centroids[i][j] - arrayOfCentroids[i][j]), 2);
      }
    }

  difference /= m_NumberOfClasses;

  return std::sqrt(difference);
}


} // end of namespace itk

#endif // __itkFuzzyClassifierInitializationImageFilter_txx

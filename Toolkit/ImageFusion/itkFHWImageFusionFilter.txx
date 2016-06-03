/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkFHWImageFusionFilter.h,v $
  Language:  C++
  Date:      $Date: 2009-07-24 11:01:02 $
  Version:   $Revision: 1.00 $

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#ifndef __itkFHWImageFusionFilter_txx
#define __itkFHWImageFusionFilter_txx

#include "itkFHWImageFusionFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace itk
{

/**
 * Constructor
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/  >
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::FHWImageFusionFilter()
{
  this->SetNumberOfRequiredInputs( 4 );
  this->InPlaceOff();

  m_Cnt1 = 0; 
  m_Cnt2 = 0; 
  m_Cnt3 = 0; 
  m_Cnt = 0;

  m_EpsilonGreyValue = 0;
  m_EpsilonGradientMagnitude = 0;
  m_TrustedDeltaGradientMagnitude = 0;
  m_SigmoidMultiplier = 0;
}


/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/  >
void
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::SetInput1( const TInputImage1 * image1 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(0, const_cast<TInputImage1 *>( image1 ));
}


/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/ >
void
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::SetInput2( const TInputImage2 * image2 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(1, const_cast<TInputImage2 *>( image2 ));
}

/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/ >
void
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::SetInput3( const TInputImage3 * image3 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(2, const_cast<TInputImage3 *>( image3 ));
}

/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/ >
void
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::SetInput4( const TInputImage4 * image4 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(3, const_cast<TInputImage4 *>( image4 ));
}

/**
 * ThreadedGenerateData Performs the pixel-wise addition
 */
template <class TInputImage1, class TInputImage2, class TInputImage3, class TInputImage4, 
          class TOutputImage/*, class TFunction*/   >
void
FHWImageFusionFilter<TInputImage1,TInputImage2,TInputImage3,TInputImage4,TOutputImage/*,TFunction*/>
::ThreadedGenerateData( const OutputImageRegionType &outputRegionForThread,
                        itk::ThreadIdType threadId)
{
  // We use dynamic_cast since inputs are stored as DataObjects.  The
  // ImageToImageFilter::GetInput(int) always returns a pointer to a
  // TInputImage1 so it cannot be used for the second input.
  Input1ImagePointer inputPtr1
    = dynamic_cast<const TInputImage1*>(ProcessObject::GetInput(0));
  Input2ImagePointer inputPtr2
    = dynamic_cast<const TInputImage2*>(ProcessObject::GetInput(1));
  Input3ImagePointer inputPtr3
    = dynamic_cast<const TInputImage3*>(ProcessObject::GetInput(2));
  Input4ImagePointer inputPtr4
    = dynamic_cast<const TInputImage4*>(ProcessObject::GetInput(3));
  OutputImagePointer outputPtr = this->GetOutput(0);
  
  ImageRegionConstIterator<TInputImage1> inputIt1(inputPtr1, outputRegionForThread);
  ImageRegionConstIterator<TInputImage2> inputIt2(inputPtr2, outputRegionForThread);
  ImageRegionConstIterator<TInputImage3> inputIt3(inputPtr3, outputRegionForThread);
  ImageRegionConstIterator<TInputImage4> inputIt4(inputPtr4, outputRegionForThread);

  ImageRegionIterator<TOutputImage> outputIt(outputPtr, outputRegionForThread);

  ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

  inputIt1.GoToBegin();
  inputIt2.GoToBegin();
  inputIt3.GoToBegin();
  inputIt4.GoToBegin();
  outputIt.GoToBegin();

  typename TOutputImage::SizeType size = outputPtr->GetLargestPossibleRegion().GetSize();
  double fGV_HE, fGV_LE, fGM_HE, fGM_LE;
  double fdGV, fdGM, fWeight;

  m_Cnt1 = 0; m_Cnt2 = 0; m_Cnt3 = 0;
  m_Cnt = size[0] * size[1] * size[2];

  while( !inputIt1.IsAtEnd() ) 
    {
    fGV_HE = inputIt1.Get();        fGV_LE = inputIt2.Get();
    fGM_HE = inputIt3.Get();        fGM_LE = inputIt4.Get();
	
	fdGV = ( fGV_HE - fGV_LE ); // calc deltaGV
    fdGM = ( fGM_HE - fGM_LE ); // calc deltaGM

    if ( ( abs(fdGM) > m_EpsilonGradientMagnitude ) || ( abs(fdGV) > m_EpsilonGreyValue ) ) { 
		outputIt.Set( fGV_HE ); m_Cnt3++; //artefact
	} else {
		if ( ( abs(fdGV) < m_EpsilonGreyValue ) && ( fdGM > m_TrustedDeltaGradientMagnitude ) 
			&& ( fGM_HE > m_EpsilonGradientMagnitude ) && ( fGM_LE > m_EpsilonGradientMagnitude ) ) {// transition
			fWeight = 1 / ( 1 + ( exp ( (-1) * m_SigmoidMultiplier * ( fGM_HE - m_EpsilonGradientMagnitude ) ) ) );  
		    if ( fWeight > 1 ) fWeight = 1;
			if ( fWeight < 0.5 ) fWeight = 0; 
			outputIt.Set( fWeight * fGV_LE + ( 1 - fWeight ) * fGV_HE ); m_Cnt2++;
		} else { 
			outputIt.Set( 0.5 * fGV_LE + 0.5 * fGV_HE ); m_Cnt1++; // homogeneous
		}
    }

	++inputIt4;
	++inputIt3;
    ++inputIt2;
    ++inputIt1;
    ++outputIt;
    progress.CompletedPixel(); // potential exception thrown here
    }
}

} // end namespace itk

#endif

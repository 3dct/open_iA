#ifndef __itkThresholdImageFusionFilter_txx
#define __itkThresholdImageFusionFilter_txx

#include "itkThresholdImageFusionFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkProgressReporter.h"

namespace itk
{

/**
 * Constructor
 */
template <class TInputImage1, class TInputImage2, class TOutputImage>
	ThresholdImageFusionFilter<TInputImage1, TInputImage2, TOutputImage>
	::ThresholdImageFusionFilter()

{
  this->SetNumberOfRequiredInputs( 2 );
  this->InPlaceOff();
}


/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, 
          class TOutputImage/*, class TFunction*/  >
void
ThresholdImageFusionFilter<TInputImage1,TInputImage2,TOutputImage/*,TFunction*/>
::SetInput1( const TInputImage1 * image1 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(0, const_cast<TInputImage1 *>( image1 ));
}


/**
 * Connect one of the operands for pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2,
          class TOutputImage/*, class TFunction*/ >
void
ThresholdImageFusionFilter<TInputImage1,TInputImage2,TOutputImage/*,TFunction*/>
::SetInput2( const TInputImage2 * image2 ) 
{
  // Process object is not const-correct so the const casting is required.
  this->SetNthInput(1, const_cast<TInputImage2 *>( image2 ));
}



/**
 * ThreadedGenerateData Performs the pixel-wise fusion
 */
template <class TInputImage1, class TInputImage2, 
          class TOutputImage/*, class TFunction*/   >
void
ThresholdImageFusionFilter<TInputImage1,TInputImage2,TOutputImage/*,TFunction*/>
::ThreadedGenerateData( const OutputImageRegionType &outputRegionForThread,
                        itk::ThreadIdType threadId)
{
	// We use dynamic_cast since inputs are stored as DataObjects.  The
	// ImageToImageFilter::GetInput(int) always returns a pointer to a
	// TInputImage1 so it cannot be used for the second input.
	Input1ImagePointer inputPtr1 = dynamic_cast<const TInputImage1*>(ProcessObject::GetInput(0));
	Input2ImagePointer inputPtr2 = dynamic_cast<const TInputImage2*>(ProcessObject::GetInput(1));

	ImageRegionConstIterator<TInputImage1> inputIt1(inputPtr1, outputRegionForThread);
	ImageRegionConstIterator<TInputImage2> inputIt2(inputPtr2, outputRegionForThread);

	OutputImagePointer outputPtr = this->GetOutput(0);
	ImageRegionIterator<TOutputImage> outputIt(outputPtr, outputRegionForThread);

	ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

	inputIt1.GoToBegin();
	inputIt2.GoToBegin();
	outputIt.GoToBegin();

	while( !inputIt1.IsAtEnd() ) 
	{
		Input1RealType i1, i2;
		i1 = inputIt1.Get();
		i2 = inputIt2.Get();
		if( inputIt1.Get() > m_Threshold)
			outputIt.Set( inputIt1.Get() );
		else
			outputIt.Set( inputIt2.Get() );

		++inputIt2;
		++inputIt1;
		++outputIt;
		progress.CompletedPixel();  //potential exception thrown here
	}
}

} // end namespace itk

#endif


#ifndef __itkArtifactBasedImageFusionFilter_txx
#define __itkArtifactBasedImageFusionFilter_txx

#include "itkArtifactBasedImageFusionFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkProgressReporter.h"

namespace itk
{

	/**
	* Constructor
	*/
	template <class TInputImage1, class TInputImage2, class TInputImage3, class TOutputImage>
	ArtifactBasedImageFusionFilter<TInputImage1, TInputImage2, TInputImage3, TOutputImage>
		::ArtifactBasedImageFusionFilter()

	{
		this->SetNumberOfRequiredInputs( 3 );
		this->InPlaceOff();
	}


	/**
	* Connect one of the operands for pixel-wise fusion
	*/
	template <class TInputImage1, class TInputImage2, class TInputImage3, class TOutputImage>
	void
		ArtifactBasedImageFusionFilter<TInputImage1, TInputImage2, TInputImage3, TOutputImage>
		::SetInput1( const TInputImage1 * image1 ) 
	{
		// Process object is not const-correct so the const casting is required.
		this->SetNthInput(0, const_cast<TInputImage1 *>( image1 ));
	}


	/**
	* Connect one of the operands for pixel-wise fusion
	*/
	template <class TInputImage1, class TInputImage2, class TInputImage3, class TOutputImage>
	void
		ArtifactBasedImageFusionFilter<TInputImage1, TInputImage2, TInputImage3, TOutputImage>
		::SetInput2( const TInputImage2 * image2 ) 
	{
		// Process object is not const-correct so the const casting is required.
		this->SetNthInput(1, const_cast<TInputImage2 *>( image2 ));
	}

	/**
	* Connect one of the operands for pixel-wise fusion
	*/
	template <class TInputImage1, class TInputImage2, class TInputImage3, class TOutputImage >
	void
		ArtifactBasedImageFusionFilter<TInputImage1, TInputImage2, TInputImage3, TOutputImage>
		::SetInput3( const TInputImage3 * image3 ) 
	{
		// Process object is not const-correct so the const casting is required.
		this->SetNthInput(2, const_cast<TInputImage3 *>( image3 ));
	}



	/**
	* ThreadedGenerateData Performs the pixel-wise fusion
	*/
	template <class TInputImage1, class TInputImage2, class TInputImage3, class TOutputImage>
	void
		ArtifactBasedImageFusionFilter<TInputImage1, TInputImage2, TInputImage3, TOutputImage>
		::ThreadedGenerateData( const OutputImageRegionType &outputRegionForThread,
		itk::ThreadIdType threadId)
	{
		// We use dynamic_cast since inputs are stored as DataObjects.  The
		// ImageToImageFilter::GetInput(int) always returns a pointer to a
		// TInputImage1 so it cannot be used for the second input.
		Input1ImagePointer inputPtr1 = dynamic_cast<const TInputImage1*>(ProcessObject::GetInput(0));
		Input2ImagePointer inputPtr2 = dynamic_cast<const TInputImage2*>(ProcessObject::GetInput(1));
		Input3ImagePointer inputPtr3 = dynamic_cast<const TInputImage2*>(ProcessObject::GetInput(2));

		ImageRegionConstIterator<TInputImage1> inputIt1(inputPtr1, outputRegionForThread);
		ImageRegionConstIterator<TInputImage2> inputIt2(inputPtr2, outputRegionForThread);
		ImageRegionConstIterator<TInputImage3> inputIt3(inputPtr3, outputRegionForThread);

		OutputImagePointer outputPtr = this->GetOutput(0);
		ImageRegionIterator<TOutputImage> outputIt(outputPtr, outputRegionForThread);

		ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

		inputIt1.GoToBegin();
		inputIt2.GoToBegin();
		inputIt3.GoToBegin();
		outputIt.GoToBegin();
		Input1RealType i1, i2, i3, coef;
		while( !inputIt1.IsAtEnd() ) 
		{	
			i1 = inputIt1.Get();
			i2 = inputIt2.Get();
			i3 = inputIt3.Get();
			//outputIt.Set( i2 - i1 );
			if( i3 > m_ThresholdRange[1] || i3 < -m_ThresholdRange[1])
				outputIt.Set( i1 );
			else if( i3 < m_ThresholdRange[0] && i3 > -m_ThresholdRange[0])
				outputIt.Set( i2 );
			else
			{
				if(i3>=0)
				{
					coef = (i3 - m_ThresholdRange[0])/m_ThresholdRangeLength;
					outputIt.Set( i2 + (i1-i2)*coef );
				}
				else
				{
					coef = (-i3 - m_ThresholdRange[0])/m_ThresholdRangeLength;
					outputIt.Set( i2 + (i1-i2)*coef );
				}
			}/**/

			++inputIt1;
			++inputIt2;
			++inputIt3;
			++outputIt;
			progress.CompletedPixel();  //potential exception thrown here
		}
	}

} // end namespace itk

#endif


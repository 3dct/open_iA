#ifndef __itkMaximumDistance_h
#define __itkMaximumDistance_h

#include "itkImageToImageFilter.h"
//  Software Guide : EndCodeSnippet

//  Software Guide : BeginLatex
//
//  Next we include headers for the component filters:
//
//  Software Guide : EndLatex

//  Software Guide : BeginCodeSnippet
#include "itkBinaryThresholdImageFilter.h"

#include "itkImageRegionConstIterator.h"

//  Software Guide : EndCodeSnippet

//#include "itkNumericTraits.h"

//  Software Guide : BeginLatex
//
//  Now we can declare the filter itself.  It is within the ITK namespace,
//  and we decide to make it use the same image type for both input and
//  output, thus the template declaration needs only one parameter.
//  Deriving from \code{ImageToImageFilter} provides default behavior for
//  several important aspects, notably allocating the output image (and
//  making it the same dimensions as the input).
//
//  Software Guide : EndLatex

//  Software Guide : BeginCodeSnippet
namespace itk {

	template <class TImageType>
	class ITK_EXPORT MaximumDistance :
		public ImageToImageFilter<TImageType, TImageType>
	{
	public:
		//  Software Guide : EndCodeSnippet

		//  Software Guide : BeginLatex
		//
		//  Next we have the standard declarations, used for object creation with
		//  the object factory:
		//
		//  Software Guide : EndLatex

		//  Software Guide : BeginCodeSnippet
		typedef MaximumDistance               Self;
		typedef ImageToImageFilter<TImageType,TImageType> Superclass;
		typedef SmartPointer<Self>                        Pointer;
		typedef SmartPointer<const Self>                  ConstPointer;
		//  Software Guide : EndCodeSnippet

		/** Method for creation through object factory */
		itkNewMacro(Self);

		/** Run-time type information */
		itkTypeMacro(MaximumDistance, ImageToImageFilter);

		/** Display */
		void PrintSelf( std::ostream& os, Indent indent ) const;

		//  Software Guide : BeginLatex
		//
		//  Here we declare an alias (to save typing) for the image's pixel type,
		//  which determines the type of the threshold value.  We then use the
		//  convenience macros to define the Get and Set methods for this parameter.
		//
		//  Software Guide : EndLatex

		//  Software Guide : BeginCodeSnippet

		typedef typename TImageType::PixelType PixelType;
		typedef typename TImageType::Pointer ImagePointer;
		typedef typename TImageType::ConstPointer ImageConstPointer;
		typedef itk::ImageRegionConstIterator <TImageType> ConstIteratorType;

		itkGetMacro( Threshold, PixelType);
		itkSetMacro( Threshold, PixelType);

		//  Software Guide : EndCodeSnippet

		//!Set centre of histogram to calculate the low intensity peak
		/*!
		\param lowIntensity an double, which is taken as the range to calculate the low intensity.
		*/
		void SetCentre(double lowIntensity);

		//!Set number of histogram bins
		/*!
		\param bin an double, which is the number of bins in histogram.
		*/
		void SetBins(double bin);

		//!Get maximum distance threshold
		/*!
		\return t an int, which is the maximum distance threshold.
		*/
		void GetThreshold( int* t);

		//!Get low intensity peak
		/*!
		\return li an int, which is the low intensity peak.
		*/
		void GetLowIntensity( int* li);

		//!Get high intensity peak
		/*!
		\return hi an int, which is the high intensity peak.
		*/
		void GetHighIntensity( int* hi);

	protected:

		MaximumDistance();

		//  Software Guide : BeginLatex
		//
		//  Now we can declare the component filter types, templated over the
		//  enclosing image type:
		//
		//  Software Guide : EndLatex

		//  Software Guide : BeginCodeSnippet
	protected:



		typedef BinaryThresholdImageFilter< TImageType, TImageType > BTIFType;

		//  Software Guide : EndCodeSnippet

		void GenerateData();

	private:

		MaximumDistance(Self&);   // intentionally not implemented
		void operator=(const Self&);          // intentionally not implemented

		//  Software Guide : BeginLatex
		//
		//  The component filters are declared as data members, all using the smart
		//  pointer types.
		//
		//  Software Guide : EndLatex

		//  Software Guide : BeginCodeSnippet



		typename BTIFType::Pointer     m_BTIFFilter;

		PixelType m_Threshold, m_intensity, m_first_value, m_last_value;
		PixelType m_i, m_start, m_end, m_high_intensity, m_low_intensity, m_centre;
		double m_Bins;
		int m_high_freq, m_low_freq;
		ImageConstPointer m_InImage;
	};

} /* namespace itk */

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMaximumDistance.txx"
#endif

#endif

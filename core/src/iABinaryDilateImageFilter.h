/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/

#ifndef IABINARYDILATEIMAGEFILTER_H
#define IABINARYDILATEIMAGEFILTER_H

#include <vector>
#include <queue>
#include "itkBinaryMorphologyImageFilter.h"
#include "itkConstNeighborhoodIterator.h"

namespace itk
{

	/**
	* \brief Fast binary dilation of the label image. Adapted from ITKs binary dilation filter.
	*
	* This Filter dilates the label using parameters for number of iterations and radius for the structuring element.
	*
	* iABinaryDilateImageFilter is a binary dilation
	* morphologic operation. This implementation is based on the papers:
	*
	* L.Vincent "Morphological transformations of binary images with
	* arbitrary structuring elements", and
	*
	* N.Nikopoulos et al. "An efficient algorithm for 3d binary
	* morphological transformations with 3d structuring elements
	* for arbitrary size and shape". IEEE Transactions on Image
	* Processing. Vol. 9. No. 3. 2000. pp. 283-286.
	*
	* Gray scale images can be processed as binary images by selecting a
	* "DilateValue".  Pixel values matching the dilate value are
	* considered the "foreground" and all other pixels are
	* "background". This is useful in processing segmented images where
	* all pixels in segment #1 have value 1 and pixels in segment #2 have
	* value 2, etc. A particular "segment number" can be processed.
	* DilateValue defaults to the maximum possible value of the
	* PixelType.
	*
	* The structuring element is assumed to be composed of binary values
	* (zero or one). Only elements of the structuring element having
	* values > 0 are candidates for affecting the center pixel.  A
	* reasonable choice of structuring element is
	* itk::BinaryBallStructuringElement.
	*
	* \sa ImageToImageFilter BinaryErodeImageFilter BinaryMorphologyImageFilter
	* \ingroup ITKBinaryMathematicalMorphology
	*
	* \wiki
	* \wikiexample{Morphology/iABinaryDilateImageFilter,Dilate a binary image}
	* \endwiki
	*/
	template< class TInputImage, class TInputImageType2, class TOutputImage, class TKernel >
	class ITK_EXPORT iABinaryDilateImageFilter:
		public BinaryMorphologyImageFilter< TInputImage, TOutputImage, TKernel >
	{
	public:

		/** Extract dimension from input and output image. */
		itkStaticConstMacro(InputImageDimension, unsigned int,
			TInputImage::ImageDimension);
		itkStaticConstMacro(OutputImageDimension, unsigned int,
			TOutputImage::ImageDimension);

		/** Extract the dimension of the kernel */
		itkStaticConstMacro(KernelDimension, unsigned int,
			TKernel::NeighborhoodDimension);

		/** Convenient typedefs for simplifying declarations. */
		typedef TInputImage  InputImageType;
		typedef TOutputImage OutputImageType;
		typedef TKernel      KernelType;

		/** Standard class typedefs. */
		typedef iABinaryDilateImageFilter Self;
		typedef BinaryMorphologyImageFilter< InputImageType, OutputImageType,
			KernelType > Superclass;

		typedef SmartPointer< Self >       Pointer;
		typedef SmartPointer< const Self > ConstPointer;

		/** Method for creation through the object factory. */
		itkNewMacro(Self);

		/** Run-time type information (and related methods). */
		itkTypeMacro(iABinaryDilateImageFilter, BinaryMorphologyImageFilter);

		/** Kernel (structuring element) iterator. */
		typedef typename KernelType::ConstIterator KernelIteratorType;

		/** Image typedef support. */
		typedef typename InputImageType::PixelType                 InputPixelType;
		typedef typename OutputImageType::PixelType                OutputPixelType;
		typedef typename NumericTraits< InputPixelType >::RealType InputRealType;
		typedef typename InputImageType::OffsetType                OffsetType;
		typedef typename InputImageType::IndexType                 IndexType;
		typedef typename TOutputImage::IndexType                 IndexType2;

		typedef typename InputImageType::RegionType  InputImageRegionType;
		typedef typename OutputImageType::RegionType OutputImageRegionType;
		typedef typename InputImageType::SizeType    InputSizeType;

		void SetInput2( const TInputImageType2 * image2 ) {   // Process object is not const-correct so the const casting is required.
			this->SetNthInput(1, const_cast<TInputImageType2 *>( image2 )); }; //Method for setting second input image. Original label image.

			//FibreSegmentMapType GetFinalFibreList( ){ return m_FinalFibreList;};

			/** Set the value in the image to consider as "foreground". Defaults to
			* maximum value of PixelType. This is an alias to the
			* ForegroundValue in the superclass. */
			void SetDilateValue(const InputPixelType & value)
			{ this->SetForegroundValue(value); }

			/** Get the value in the image considered as "foreground". Defaults to
			* maximum value of PixelType. This is an alias to the
			* ForegroundValue in the superclass. */
			InputPixelType GetDilateValue() const
			{ return this->GetForegroundValue(); }


	protected:
		iABinaryDilateImageFilter();
		virtual ~iABinaryDilateImageFilter(){}
		void PrintSelf(std::ostream & os, Indent indent) const;

		void GenerateData();

		// type inherited from the superclass
		typedef typename Superclass::NeighborIndexContainer NeighborIndexContainer;
	private:
		iABinaryDilateImageFilter(const Self &); //purposely not implemented
		void operator=(const Self &);          //purposely not implemented
	};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "iABinaryDilateImageFilter.hxx"
#endif

#endif

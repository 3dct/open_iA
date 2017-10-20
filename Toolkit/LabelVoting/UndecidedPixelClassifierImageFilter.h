/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <itkImageToImageFilter.h>

template< typename TInputImage, typename TOutputImage = TInputImage >
class UndecidedPixelClassifierImageFilter :
	public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	typedef UndecidedPixelClassifierImageFilter                  Self;
	typedef itk::ImageToImageFilter< TInputImage, TOutputImage > Superclass;
	typedef itk::SmartPointer< Self >                            Pointer;
	typedef itk::SmartPointer< const Self >                      ConstPointer;
	itkNewMacro(Self);
	itkTypeMacro(UndecidedPixelClassifierImageFilter, ImageToImageFilter);
	typedef typename TOutputImage::PixelType OutputPixelType;
	typedef typename TInputImage::PixelType  InputPixelType;
	itkStaticConstMacro(InputImageDimension, int, TInputImage::ImageDimension);
	itkStaticConstMacro(ImageDimension, int, TOutputImage::ImageDimension);

	typedef TInputImage                           InputImageType;
	typedef TOutputImage                          OutputImageType;
	typedef typename InputImageType::ConstPointer InputImagePointer;
	typedef typename OutputImageType::Pointer     OutputImagePointer;
	typedef unsigned long                         LabelCountType;
	typedef typename Superclass::OutputImageRegionType OutputImageRegionType;
	typedef itk::Image<double, 3>                 DoubleImg;
	typedef itk::ImageRegionConstIterator<DoubleImg> ConstDblIt;

	void SetUndecidedPixelLabel(const OutputPixelType l)
	{
		m_undecidedPixelLabel = l;
		this->Modified();
	}

	OutputPixelType GetUndecidedPixelLabel() const
	{
		return m_undecidedPixelLabel;
	}

	void SetRadius(itk::Size<TInputImage::ImageDimension> radius)
	{
		m_radius = radius;
	}

	//! set probabilities for each label for one input
	//! in order for probability-related functionality to work, this method needs to be
	//! called once with each input idx
	void SetProbabilityImages(int inputIdx, std::vector<DoubleImg::Pointer> const & probImgs)
	{
		m_probImgs.insert(std::make_pair(inputIdx, probImgs));
	}

#ifdef ITK_USE_CONCEPT_CHECKING
	itkConceptMacro(InputConvertibleToOutputCheck, (itk::Concept::Convertible< InputPixelType, OutputPixelType >));
	itkConceptMacro(IntConvertibleToInputCheck, (itk::Concept::Convertible< int, InputPixelType >));
	itkConceptMacro(SameDimensionCheck, (itk::Concept::SameDimension< InputImageDimension, ImageDimension >));
	itkConceptMacro(InputIntCheck,(itk::Concept::IsInteger< InputPixelType >));
	itkConceptMacro(IntConvertibleToOutputPixelType, (itk::Concept::Convertible< int, OutputPixelType >));
	itkConceptMacro(InputPlusIntCheck, (itk::Concept::AdditiveOperators< InputPixelType, int >));
	itkConceptMacro(InputIncrementDecrementOperatorsCheck, (itk::Concept::IncrementDecrementOperators< InputPixelType >));
	itkConceptMacro(OutputOStreamWritableCheck, (itk::Concept::OStreamWritable< OutputPixelType >));
#endif

protected:
	UndecidedPixelClassifierImageFilter();
	virtual ~UndecidedPixelClassifierImageFilter() {}
	void BeforeThreadedGenerateData() ITK_OVERRIDE;

	void ThreadedGenerateData
		(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId) ITK_OVERRIDE;

	void PrintSelf(std::ostream &, itk::Indent) const ITK_OVERRIDE;

	InputPixelType ComputeMaximumInputValue();

private:
	UndecidedPixelClassifierImageFilter(const Self &) ITK_DELETE_FUNCTION;
	void operator=(const Self &) ITK_DELETE_FUNCTION;

	OutputPixelType m_undecidedPixelLabel;
	size_t          m_labelCount;
	itk::Size<TInputImage::ImageDimension> m_radius;

	std::map<int, std::vector<DoubleImg::Pointer> > m_probImgs;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "UndecidedPixelClassifierImageFilter.hxx"
#endif

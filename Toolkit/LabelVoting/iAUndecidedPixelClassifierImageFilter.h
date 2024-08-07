// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkImageToImageFilter.h>

//! Given a number of input images, compute a final classification based on first and second best guess.
template< typename TInputImage, typename TOutputImage = TInputImage >
class iAUndecidedPixelClassifierImageFilter :
	public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	typedef iAUndecidedPixelClassifierImageFilter                  Self;
	typedef itk::ImageToImageFilter< TInputImage, TOutputImage > Superclass;
	typedef itk::SmartPointer< Self >                            Pointer;
	typedef itk::SmartPointer< const Self >                      ConstPointer;
	itkNewMacro(Self);
	itkTypeMacro(iAUndecidedPixelClassifierImageFilter, ImageToImageFilter);
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
		m_hasUndecidedPixelLabel = true;
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

	void SetProbabilityImages(int inputIdx, std::vector<DoubleImg::Pointer> const & probImgs)
	{
		m_probImgs.insert(std::make_pair(inputIdx, probImgs));
	}

	void SetUncertaintyAsTieSolver(bool uncertaintyTieSolver)
	{
		m_uncertaintyTieSolver = uncertaintyTieSolver;
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
	iAUndecidedPixelClassifierImageFilter();
	virtual ~iAUndecidedPixelClassifierImageFilter() {}
	void BeforeThreadedGenerateData() override;

	void ThreadedGenerateData
		(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId) override;

	void PrintSelf(std::ostream &, itk::Indent) const override;

	InputPixelType ComputeMaximumInputValue();

private:
	iAUndecidedPixelClassifierImageFilter(const Self &) =delete;
	void operator=(const Self &) =delete;

	OutputPixelType m_undecidedPixelLabel;
	bool m_hasUndecidedPixelLabel;
	size_t m_labelCount;
	itk::Size<TInputImage::ImageDimension> m_radius;
	std::map<int, std::vector<DoubleImg::Pointer> > m_probImgs;
	bool m_uncertaintyTieSolver;
};

#include "iAUndecidedPixelClassifierImageFilter.hxx"

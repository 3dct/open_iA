// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageToImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

enum VotingRule
{
	SumRule,
	MaxRule,
	MinRule,
	MedianRule,
	MajorityVoteRule		// includes weighted voting, use SetWeights
};

//! Filter implementing probabilistic voting based on a given voting rule
template< typename TInputImage, typename TOutputImage = TInputImage >
class iAProbabilisticVotingImageFilter :
	public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	typedef iAProbabilisticVotingImageFilter                       Self;
	typedef itk::ImageToImageFilter< TInputImage, TOutputImage > Superclass;
	typedef itk::SmartPointer< Self >                            Pointer;
	typedef itk::SmartPointer< const Self >                      ConstPointer;
	itkNewMacro(Self);
	itkTypeMacro(iAProbabilisticVotingImageFilter, ImageToImageFilter);
	typedef typename TOutputImage::PixelType OutputPixelType;
	typedef typename TInputImage::PixelType  InputPixelType;
	itkStaticConstMacro(InputImageDimension, int, TInputImage::ImageDimension);
	itkStaticConstMacro(ImageDimension, int, TOutputImage::ImageDimension);
	typedef TInputImage                           InputImageType;
	typedef TOutputImage                          OutputImageType;
	typedef typename InputImageType::ConstPointer InputImagePointer;
	typedef typename OutputImageType::Pointer     OutputImagePointer;
	typedef typename Superclass::OutputImageRegionType OutputImageRegionType;
	typedef itk::Image<double, 3>                 DoubleImg;
	typedef itk::ImageRegionConstIterator<DoubleImg> ConstDblIt;

	void SetProbabilityImages(size_t inputIdx, std::vector<DoubleImg::Pointer> const & probImgs)
	{
		m_probImgs.insert(std::make_pair(inputIdx, probImgs));
	}

	void SetVotingRule(VotingRule rule)
	{
		m_votingRule = rule;
	}

	void SetUndecidedUncertaintyThreshold(double uncertainty)
	{
		m_undecidedUncertaintyThresh = uncertainty;
	}

	void SetWeights(std::vector<double> weights)
	{
		m_weights = weights;
	}

	double GetUndecided() const
	{
		return m_undecidedPixels;
	}

#ifdef ITK_USE_CONCEPT_CHECKING
	itkConceptMacro(IntConvertibleToOutputPixelType, (itk::Concept::Convertible< int, OutputPixelType >));
	itkConceptMacro(OutputOStreamWritableCheck, (itk::Concept::OStreamWritable< OutputPixelType >));
#endif

protected:
	iAProbabilisticVotingImageFilter();
	virtual ~iAProbabilisticVotingImageFilter() {}
	void BeforeThreadedGenerateData() override;
	void ThreadedGenerateData
		(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId) override;

	void PrintSelf(std::ostream &, itk::Indent) const override;

private:
	iAProbabilisticVotingImageFilter(const Self &) =delete;
	void operator=(const Self &) =delete;

	std::map<size_t, std::vector<DoubleImg::Pointer> > m_probImgs;
	std::vector<double> m_weights;
	VotingRule m_votingRule;
	size_t m_labelCount;
	size_t m_numberOfClassifiers;
	double m_undecidedUncertaintyThresh;
	double m_undecidedPixels;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAProbabilisticVotingImageFilter.hxx"
#endif

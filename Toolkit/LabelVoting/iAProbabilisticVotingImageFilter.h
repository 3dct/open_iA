/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

enum VotingRule
{
	SumRule,
	MaxRule,
	MinRule,
	MedianRule,
	MajorityVoteRule		// includes weighted voting, use SetWeights
};

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

	void SetProbabilityImages(int inputIdx, std::vector<DoubleImg::Pointer> const & probImgs)
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

	std::map<int, std::vector<DoubleImg::Pointer> > m_probImgs;
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

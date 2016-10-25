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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
/*
 This file was originally released as part of the ITK project,
 under the Apache license (see below).
 It has been adapted; for information on the specific changes,
 please compare this file to the file
 Modules\Segmentation\LabelVoting\include\itkLabelVotingImageFilter.hxx
 as included in the ITK release 4.10
*/
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
#pragma once

#include "ParametrizableLabelVotingImageFilter.h"

#include "itkImageRegionIterator.h"
#include "itkProgressReporter.h"

#include "itkMath.h"

template< typename TInputImage, typename TOutputImage >
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::ParametrizableLabelVotingImageFilter():
	m_AbsMinPercentage(-1),
	m_MinDiffPercentage(-1),
	m_MinRatio(-1)
{
	this->m_HasLabelForUndecidedPixels = false;
	this->m_LabelForUndecidedPixels = 0;
	this->m_TotalLabelCount = 0;
}

template< typename TInputImage, typename TOutputImage >
void
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "m_HasLabelForUndecidedPixels = "
		<< this->m_HasLabelForUndecidedPixels << std::endl;
	os << indent << "m_LabelForUndecidedPixels = "
		<< this->m_LabelForUndecidedPixels << std::endl;
}

template< typename TInputImage, typename TOutputImage >
typename ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >::InputPixelType
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::ComputeMaximumInputValue()
{
	InputPixelType maxLabel = 0;

	typedef itk::ImageRegionConstIterator< TInputImage > IteratorType;

	// Record the number of input files.
	const size_t numberOfInputFiles = this->GetNumberOfIndexedInputs();

	for (size_t i = 0; i < numberOfInputFiles; ++i)
	{
		const InputImageType *inputImage = this->GetInput(i);
		IteratorType          it(inputImage, inputImage->GetBufferedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			maxLabel = std::max(maxLabel, it.Get());
		}
	}

	return maxLabel;
}

template <typename TInputImage, typename TOutputImage>
typename TOutputImage::Pointer CreateImage(typename itk::SmartPointer<const TInputImage> otherImg)
{
	typename TOutputImage::Pointer image = TOutputImage::New();
	typename TOutputImage::RegionType reg(
		otherImg->GetLargestPossibleRegion().GetIndex(),
		otherImg->GetLargestPossibleRegion().GetSize()
	);
	double outputOrigin[3];
	outputOrigin[0] = outputOrigin[1] = outputOrigin[2] = 0;
	image->SetOrigin(outputOrigin);
	image->SetRegions(reg);
	image->Allocate();
	image->FillBuffer(0);
	image->SetSpacing(otherImg->GetSpacing());
	return image;
}

template< typename TInputImage, typename TOutputImage >
void
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();

	// determine the maximum label in all input images
	this->m_TotalLabelCount =
		static_cast<size_t>(this->ComputeMaximumInputValue()) + 1;

	if (!this->m_HasLabelForUndecidedPixels)
	{
		if (this->m_TotalLabelCount > itk::NumericTraits<OutputPixelType>::max())
		{
			itkWarningMacro("No new label for undecided pixels, using zero.");
		}
		this->m_LabelForUndecidedPixels = static_cast<OutputPixelType>(this->m_TotalLabelCount);
	}

	// Allocate the output image.
	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();

	// Allocate the images containing the "decision numbers"
	itk::SmartPointer<const TInputImage> in0 = this->GetInput(0);
	m_imgAbsMinPerc = CreateImage<TInputImage, DoubleImg>(this->GetInput(0));
	m_imgMinDiffPerc = CreateImage<TInputImage, DoubleImg>(this->GetInput(0));
	m_imgMinRatio = CreateImage<TInputImage, DoubleImg>(this->GetInput(0));
}

template< typename TInputImage, typename TOutputImage >
void ParametrizableLabelVotingImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

	typedef itk::ImageRegionConstIterator< TInputImage > IteratorType;
	typedef itk::ImageRegionIterator< TOutputImage >     OutIteratorType;

	typename TOutputImage::Pointer output = this->GetOutput();

	// Record the number of input files.
	const size_t numberOfInputFiles = this->GetNumberOfIndexedInputs();

	//  create and initialize all input image iterators
	IteratorType *it = new IteratorType[numberOfInputFiles];
	for (size_t i = 0; i < numberOfInputFiles; ++i)
	{
		it[i] = IteratorType(this->GetInput(i),	outputRegionForThread);
	}

	unsigned int *votesByLabel = new unsigned int[this->m_TotalLabelCount];

	OutIteratorType out = OutIteratorType(output, outputRegionForThread);
	typedef itk::ImageRegionIterator<DoubleImg> DoubleOutIteratorType;
	DoubleOutIteratorType absOut(m_imgAbsMinPerc, outputRegionForThread);
	DoubleOutIteratorType diffOut(m_imgMinDiffPerc, outputRegionForThread);
	DoubleOutIteratorType ratioOut(m_imgMinRatio, outputRegionForThread);
	absOut.GoToBegin();
	diffOut.GoToBegin();
	ratioOut.GoToBegin();
	if (!m_avgEntropy.empty())
	{
		for (int i = 0; i < m_TotalLabelCount; ++i)
		{
			if (m_avgEntropy.count(i) == 0)
			{
				DEBUG_LOG(QString("ERROR! No Average Entropy defined for label %1").arg(i));
			}
		}
	}
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		// reset number of votes per label for all labels
		std::fill_n(votesByLabel, this->m_TotalLabelCount, 0);

		// count number of votes for the labels
		for (unsigned int i = 0; i < numberOfInputFiles; ++i)
		{
			const InputPixelType label = it[i].Get();
			if (itk::NumericTraits<InputPixelType>::IsNonnegative(label))
			{
				++votesByLabel[label];
			}
			++(it[i]);
		}

		// determine the label with the most votes for this pixel
		int startIdx = 0;
		while (m_avgEntropy.count(startIdx) == 1 && m_avgEntropy[startIdx] < m_MinAvgEntropy)
		{
			startIdx++;
		}
		if (startIdx >= this->m_TotalLabelCount)
		{
			out.Set(m_LabelForUndecidedPixels);
			continue;
		}
		out.Set(startIdx);
		unsigned int firstBestGuessVotes = votesByLabel[0];
		unsigned int secondBestGuessVotes = 0;
		for (size_t l = startIdx+1; l < this->m_TotalLabelCount; ++l)
		{
			if (m_avgEntropy.count(l) == 1 && m_avgEntropy[l] < m_MinAvgEntropy)
			{
				continue;
			}
			if (votesByLabel[l] > firstBestGuessVotes)
			{
				firstBestGuessVotes = votesByLabel[l];
				out.Set(static_cast<OutputPixelType>(l));
			}
			else if (votesByLabel[l] > secondBestGuessVotes)
			{
				secondBestGuessVotes = votesByLabel[l];
			}
			else
			{
				if (votesByLabel[l] == firstBestGuessVotes)
				{
					out.Set(m_LabelForUndecidedPixels);
				}
			}
		}
		double firstBestGuessPercentage = static_cast<double>(firstBestGuessVotes) / numberOfInputFiles;
		double secondBestGuessPercentage = static_cast<double>(secondBestGuessVotes) / numberOfInputFiles;
		if (m_AbsMinPercentage >= 0 && firstBestGuessPercentage < m_AbsMinPercentage)
		{
			out.Set(this->m_LabelForUndecidedPixels);
		}
		if (m_MinDiffPercentage >= 0 &&	(firstBestGuessPercentage - secondBestGuessPercentage) < m_MinDiffPercentage)
		{
			out.Set(this->m_LabelForUndecidedPixels);
		}
		if (m_MinRatio >= 0 && secondBestGuessVotes > 0 && (firstBestGuessVotes / secondBestGuessVotes) < m_MinRatio)
		{
			out.Set(this->m_LabelForUndecidedPixels);
		}
		absOut.Set(firstBestGuessPercentage);
		++absOut;
		diffOut.Set(firstBestGuessPercentage - secondBestGuessPercentage);
		++diffOut;
		// if secondBestGuessVotes = 1 and no other votes, then ratio = numberOfInputFiles; for secondBestGuessVotes = 0 it thus should be slightly higher
		ratioOut.Set(secondBestGuessVotes > 0 ? (firstBestGuessVotes / secondBestGuessVotes) : numberOfInputFiles+1 );
		++ratioOut;
		progress.CompletedPixel();
	}

	delete[] it;
	delete[] votesByLabel;
}

template< typename TInputImage, typename TOutputImage >
void ParametrizableLabelVotingImageFilter<TInputImage, TOutputImage>::SetProbabilityImages(
	int inputIdx,
	std::vector<typename DoubleImg::Pointer> probImgs)
{
	m_probImgs.insert(inputIdx, probImgs);

	//  create and initialize all input image iterators
	typedef itk::ImageRegionConstIterator<DoubleImg> ConstDblIt;
	
	typedef fhw::EntropyImageFilter<DoubleImg, DoubleImg> EntropyFilter;
	typedef itk::StatisticsImageFilter<DoubleImg> MeanFilter;
	auto entropyFilter = EntropyFilter::New();
	for (int i = 0; i < probImgs.size(); ++i)
	{
		entropyFilter->SetInput(i, probImgs[i]);
	}
	entropyFilter->SetNormalize(true);
	entropyFilter->Update();
	auto meanFilter = MeanFilter::New();
	meanFilter->SetInput(entropyFilter->GetOutput());
	meanFilter->Update();
	double avgEntropy = meanFilter->GetMean();
	m_avgEntropy.insert(inputIdx, avgEntropy);
}
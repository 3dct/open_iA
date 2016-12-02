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

#include <itkImageRegionIterator.h>
#include <itkMath.h>
#include <itkProgressReporter.h>
#include <itkStatisticsImageFilter.h>

#include "iAMathUtility.h"

template< typename TInputImage, typename TOutputImage >
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::ParametrizableLabelVotingImageFilter():
	m_AbsMinPercentage(-1),
	m_MinDiffPercentage(-1),
	m_MinRatio(-1),
	m_MaxPixelEntropy(-1),
	m_weightType(Equal)
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
			DEBUG_LOG("No new label for undecided pixels, using zero.");
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
	m_imgPixelEntropy = CreateImage<TInputImage, DoubleImg>(this->GetInput(0));
	
	if (m_probImgs.size() == 0)
	{
		m_MaxPixelEntropy = -1;
		if (m_weightType == Certainty || m_weightType == FBGSBGDiff)
		{
			DEBUG_LOG("Weight Type set to Certainty/FBGSBGDiff, but no probability images given! Using equal weights.");
			m_weightType = Equal;
		}
	}

	if (m_inputLabelWeightMap.empty() && m_weightType == LabelBased)
	{
		DEBUG_LOG("Weight Type is set to LabelBased, but no input/label to weight map given! Using equal weights.");
		m_weightType = Equal;
	}
}

template< typename TInputImage, typename TOutputImage >
void ParametrizableLabelVotingImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

	typedef itk::ImageRegionConstIterator< TInputImage > IteratorType;
	typedef itk::ImageRegionIterator< TOutputImage >     OutIteratorType;
	typedef itk::ImageRegionIterator<DoubleImg> DoubleOutIteratorType;

	typename TOutputImage::Pointer output = this->GetOutput();

	// Record the number of input files.
	const size_t numberOfInputFiles = this->GetNumberOfIndexedInputs();

	//  create and initialize all input image iterators
	std::vector<IteratorType> it;
	std::vector<std::vector<ConstDblIt> > probIt;
	for (size_t i = 0; i < numberOfInputFiles; ++i)
	{
		it.push_back(IteratorType(GetInput(i),	outputRegionForThread));
		if (m_MaxPixelEntropy >= 0 || m_weightType == Certainty || m_weightType == FBGSBGDiff)
		{
			probIt.push_back(std::vector<ConstDblIt>());
			for (size_t l = 0; l < m_TotalLabelCount; ++l)
			{
				probIt[i].push_back(ConstDblIt(m_probImgs[i][l], outputRegionForThread));
				probIt[i][l].GoToBegin();
			}
		}
	}

	float *votesByLabel = new float[m_TotalLabelCount];

	OutIteratorType out = OutIteratorType(output, outputRegionForThread);
	DoubleOutIteratorType absOut(m_imgAbsMinPerc, outputRegionForThread);
	DoubleOutIteratorType diffOut(m_imgMinDiffPerc, outputRegionForThread);
	DoubleOutIteratorType ratioOut(m_imgMinRatio, outputRegionForThread);
	DoubleOutIteratorType entropyOut(m_imgPixelEntropy, outputRegionForThread);
	absOut.GoToBegin();
	diffOut.GoToBegin();
	ratioOut.GoToBegin();
	entropyOut.GoToBegin();
	double limit = -std::log(1.0 / numberOfInputFiles);
	double normalizeFactor = 1 / limit;
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		// reset number of votes per label for all labels
		std::fill_n(votesByLabel, m_TotalLabelCount, 0.0);

		// count number of votes for the labels
		int consideredFiles = 0;
		double avgPixelEntropy = 0;
		for (unsigned int i = 0; i < numberOfInputFiles; ++i)
		{
			const InputPixelType label = it[i].Get();
			++(it[i]);

			if (!m_inputLabelVotersSet.empty() &&
				m_inputLabelVotersSet.count(std::make_pair(label, i)) == 0)
			{
				continue;
			}
			double entropy = 0.0;
			// calculate entropy of the current pixel for each input file
			double pixelFBG = 0, pixelSBG = 0;
			if (m_MaxPixelEntropy >= 0 || m_weightType == Certainty || m_weightType == FBGSBGDiff)
			{
				double probSum = 0.0;
				for (unsigned int l = 0; l < m_TotalLabelCount; ++l)
				{
					if (probIt[i][l].IsAtEnd())
					{
						DEBUG_LOG("Prob It at end.");
					}
					const double probValue = probIt[i][l].Get();
					++(probIt[i][l]);
					probSum += probValue;
					if (probValue > 0)
					{
						entropy += (probValue * std::log(probValue));
					}
					if (probValue > pixelFBG)
					{
						pixelSBG = pixelFBG;
						pixelFBG = probValue;
					}
					else if (probValue > pixelSBG)
					{
						pixelSBG = probValue;
					}
				}
				entropy = -entropy;
				/*
				assert(entropy >= -0.000000001 && entropy <= limit + 0.000000001);
				assert(probSum >= 0.999999999 && probSum <= 1.000000001);
				*/
				entropy = clamp(0.0, limit, entropy);
				entropy = entropy * normalizeFactor;
				avgPixelEntropy += entropy;
				if (m_MaxPixelEntropy >= 0 && entropy > m_MaxPixelEntropy)
				{
					continue;
				}
			}
			consideredFiles++;
			if (itk::NumericTraits<InputPixelType>::IsNonnegative(label))
			{
				switch (m_weightType)
				{
					case Equal: votesByLabel[label] += 1.0; break;
					case LabelBased: votesByLabel[label] += m_inputLabelWeightMap[std::make_pair(label, i)]; break;
					case Certainty: votesByLabel[label] += (1.0 - entropy);
					case FBGSBGDiff: votesByLabel[label] += (pixelFBG - pixelSBG);
				}
				
			}
		}
		if (consideredFiles == 0)
		{
			out.Set(m_LabelForUndecidedPixels);
			continue;
		}

		avgPixelEntropy = avgPixelEntropy / numberOfInputFiles;
		// determine the label with the most votes for this pixel

		out.Set(0);
		unsigned int firstBestGuessLabel = 0;
		float firstBestGuessVotes = votesByLabel[0];
		for (size_t l = 1; l < m_TotalLabelCount; ++l)
		{
			if (votesByLabel[l] > firstBestGuessVotes)
			{
				firstBestGuessVotes = votesByLabel[l];
				firstBestGuessLabel = l;
				out.Set(static_cast<OutputPixelType>(l));
			} else if (votesByLabel[l] == firstBestGuessVotes)
			{
				out.Set(m_LabelForUndecidedPixels);
			}
		}
		float secondBestGuessVotes = 0;
		for (size_t l = 0; l < m_TotalLabelCount; ++l)
		{
			if (l != firstBestGuessLabel &&
				votesByLabel[l] > secondBestGuessVotes)
			{
				secondBestGuessVotes = votesByLabel[l];
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
		if (m_MinRatio >= 0 && secondBestGuessVotes > 0 && (static_cast<double>(firstBestGuessVotes) / secondBestGuessVotes) < m_MinRatio)
		{
			out.Set(this->m_LabelForUndecidedPixels);
		}
		absOut.Set(firstBestGuessPercentage);
		++absOut;
		diffOut.Set(firstBestGuessPercentage - secondBestGuessPercentage);
		++diffOut;
		// if secondBestGuessVotes = 1 and no other votes, then ratio = numberOfInputFiles-1
		ratioOut.Set(secondBestGuessVotes > 0 ? (firstBestGuessVotes / secondBestGuessVotes) : numberOfInputFiles );
		++ratioOut;
		entropyOut.Set(avgPixelEntropy);
		++entropyOut;
		progress.CompletedPixel();
	}
	delete[] votesByLabel;
}

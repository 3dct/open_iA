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
::ParametrizableLabelVotingImageFilter()
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
}

template< typename TInputImage, typename TOutputImage >
void
ParametrizableLabelVotingImageFilter< TInputImage, TOutputImage >
::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
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
		it[i] = IteratorType(this->GetInput(i),
			outputRegionForThread);
	}

	unsigned int *votesByLabel = new unsigned int[this->m_TotalLabelCount];

	OutIteratorType out = OutIteratorType(output, outputRegionForThread);
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
		out.Set(0);
		unsigned int maxVotes = votesByLabel[0];
		for (size_t l = 1; l < this->m_TotalLabelCount; ++l)
		{
			if (votesByLabel[l] > maxVotes)
			{
				maxVotes = votesByLabel[l];
				out.Set(static_cast<OutputPixelType>(l));
			}
			else
			{
				if (votesByLabel[l] == maxVotes)
				{
					out.Set(this->m_LabelForUndecidedPixels);
				}
			}
		}
		if ((static_cast<double>(maxVotes) / numberOfInputFiles) <
			m_DecisionMinimumPercentage)
		{
			out.Set(this->m_LabelForUndecidedPixels);
		}
		progress.CompletedPixel();
	}

	delete[] it;
	delete[] votesByLabel;
}

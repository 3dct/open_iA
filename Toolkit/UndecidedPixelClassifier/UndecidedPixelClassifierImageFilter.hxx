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

#include "UndecidedPixelClassifierImageFilter.h"

#include <itkImageRegionIterator.h>
#include <itkConstNeighborhoodIterator.h>
#include <itkMath.h>
#include <itkProgressReporter.h>
#include <itkStatisticsImageFilter.h>

#include "iAMathUtility.h"

template< typename TInputImage, typename TOutputImage >
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::UndecidedPixelClassifierImageFilter():
	m_HasUndecidedPixelLabel(false),
	m_UndecidedPixelLabel(0),
	m_TotalLabelCount(0)
{
	m_radius.Fill(1);
}

template< typename TInputImage, typename TOutputImage >
void
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "m_HasUndecidedPixelLabel = " << this->m_HasUndecidedPixelLabel << std::endl;
	os << indent << "m_UndecidedPixelLabel = " << this->m_UndecidedPixelLabel << std::endl;
}

template< typename TInputImage, typename TOutputImage >
typename UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >::InputPixelType
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::ComputeMaximumInputValue()
{
	typedef itk::ImageRegionConstIterator< TInputImage > IteratorType;
	const InputImageType *inputImage = this->GetInput(0);
	IteratorType          it(inputImage, inputImage->GetBufferedRegion());
	InputPixelType maxLabel = 0;
	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		maxLabel = std::max(maxLabel, it.Get());
	}
	return maxLabel;
}

template< typename TInputImage, typename TOutputImage >
void
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();
	this->m_TotalLabelCount = static_cast<size_t>(this->ComputeMaximumInputValue());

	if (!this->m_HasUndecidedPixelLabel)
	{
		this->m_UndecidedPixelLabel = static_cast<OutputPixelType>(this->m_TotalLabelCount);
	}
	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();
}

template< typename TInputImage, typename TOutputImage >
void UndecidedPixelClassifierImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	if (this->GetNumberOfIndexedInputs() != 1)
	{
		DEBUG_LOG("Expected exactly 1 input!");
		return;
	}
	if (m_probImgs.size() == 0)
	{
		DEBUG_LOG("No probability images given!");
		return;
	}

	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
	typedef itk::ConstNeighborhoodIterator<DoubleImg>    DblConstNeighborIt;

	auto output = this->GetOutput();
	std::vector<DblConstNeighborIt> probIt;
	auto it = itk::ConstNeighborhoodIterator<TInputImage>(m_radius, this->GetInput(i), outputRegionForThread);
	for (size_t l = 0; l < m_probImgs.size(); ++l)
	{
		probIt.push_back(DblConstNeighborIt(m_radius, m_probImgs[l], outputRegionForThread));
		probIt[l].GoToBegin();
	}
	auto out = itk::ImageRegionIterator<TOutputImage>(output, outputRegionForThread);
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		if (*it == m_UndecidedPixelLabel) // only change currently undecided
		{
			std::vector<int> m_labelFreq;

		}
		else
		{
			out.Set(it.Get());
		}
		progress.CompletedPixel();
	}
	delete[] votesByLabel;
}

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAEntropyImageFilter.h"

#include <itkImageRegionIterator.h>
#include <itkMath.h>
#include <itkProgressReporter.h>

#include "iAMathUtility.h"


template< typename TInputImage, typename TOutputImage >
iAEntropyImageFilter< TInputImage, TOutputImage >::iAEntropyImageFilter():
	m_normalize(false)
{
}

template< typename TInputImage, typename TOutputImage >
void iAEntropyImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
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
iAEntropyImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();
	// Allocate the output image.
	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();
}

template< typename TInputImage, typename TOutputImage >
void iAEntropyImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId)
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
		it[i].GoToBegin();
	}
	
	double limit = -std::log(1.0 / numberOfInputFiles);
	double normalizeFactor = m_normalize ? 1 / limit : 1;

	OutIteratorType out = OutIteratorType(output, outputRegionForThread);
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		double entropy = 0.0;
		double probSum = 0.0;
		for (unsigned int i = 0; i < numberOfInputFiles; ++i)
		{
			const InputPixelType probValue = it[i].Get();
			probSum += probValue;
			if (probValue > 0)
			{
				entropy += (probValue * std::log(probValue));
			}
			++(it[i]);
		}
		entropy = -entropy;
		//assert(entropy >= -0.000000001 && entropy <= limit + 0.000000001);
		//assert(probSum >= 0.999999999 && probSum <= 1.000000001);
		entropy = clamp(0.0, limit, entropy);
		entropy = entropy * normalizeFactor;
		out.Set(entropy);
		progress.CompletedPixel();
	}

	delete[] it;
}

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

#include "iAConsole.h"
#include "iAMathUtility.h"

#include <QString>

template< typename TInputImage, typename TOutputImage >
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::UndecidedPixelClassifierImageFilter():
	m_undecidedPixelLabel(0),
	m_labelCount(0)
{
	m_radius.Fill(1);
}

template< typename TInputImage, typename TOutputImage >
void
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "m_undecidedPixelLabel = " << this->m_undecidedPixelLabel << std::endl;
}

template< typename TInputImage, typename TOutputImage >
typename UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >::InputPixelType
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::ComputeMaximumInputValue()
{
	InputPixelType maxLabel = 0;
	itk::ImageRegionConstIterator<TInputImage> it(GetInput(0), GetInput(0)->GetBufferedRegion());
	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		if (it.Get() > maxLabel)
		{
			maxLabel = it.Get();
		}
	}
	return maxLabel;
}

template< typename TInputImage, typename TOutputImage >
void
UndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();
	m_labelCount = static_cast<size_t>(this->ComputeMaximumInputValue());
	m_undecidedPixelLabel = static_cast<OutputPixelType>(this->m_labelCount);
	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();
}

template< typename TInputImage, typename TOutputImage >
void UndecidedPixelClassifierImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	const size_t numberOfInputFiles = m_probImgs.size();
	if (numberOfInputFiles < 2)
	{
		DEBUG_LOG("Expected at least 2 input images!");
		return;
	}
	if (m_probImgs.size() == 0)
	{
		DEBUG_LOG("No probability images given!");
		return;
	}

	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());
	
	typedef itk::ConstNeighborhoodIterator<TInputImage> IntConstNeighborIt;
	typedef itk::ConstNeighborhoodIterator<DoubleImg>   DblConstNeighborIt;

	auto output = GetOutput();
	
	IntConstNeighborIt it(m_radius, GetInput(0), outputRegionForThread);
	it.GoToBegin();
	std::vector<std::vector<DblConstNeighborIt> > probIt;
	for (size_t i = 0; i < numberOfInputFiles; ++i)
	{
		probIt.push_back(std::vector<DblConstNeighborIt>());
		for (size_t l = 0; l < m_labelCount; ++l)
		{
			probIt[i].push_back(DblConstNeighborIt(m_radius, m_probImgs[i][l], outputRegionForThread));
			probIt[i][l].GoToBegin();
		}
	}
	auto out = itk::ImageRegionIterator<TOutputImage>(output, outputRegionForThread);
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		if (it.GetCenterPixel() == m_undecidedPixelLabel) // only change currently undecided
		{
			std::vector<int> fbgLabelFreq(m_labelCount);
			std::vector<int> sbgLabelFreq(m_labelCount);
			std::vector<int> neiLabelFreq(m_labelCount);

			// for first/second best set:
			// for each classifier:
			//     for each probability:
			//         if higher than fbg, make new fbg, move fbg to sbg
			//          else if higher than sbg, make new sbg
			//     add fbg label to F, add sbg label to S
			//     (=increase counter for fbg and sbg label in frequency histograms F and S)
			for (size_t i = 0; i < numberOfInputFiles; ++i)
			{
				int fbgLabel = 0, sbgLabel = -1;
				for (size_t l = 1; l < m_labelCount; ++l)
				{
					if (probIt[i][l].GetCenterPixel() > probIt[i][fbgLabel].GetCenterPixel())
					{
						sbgLabel = fbgLabel;
						fbgLabel = l;
					}
					else if (sbgLabel == -1 || probIt[i][l].GetCenterPixel() > probIt[i][sbgLabel].GetCenterPixel())
					{
						sbgLabel = l;
					}
				}
				++fbgLabelFreq[fbgLabel];
				++sbgLabelFreq[sbgLabel];
			}

			// for "best guess of each classifier in neighbourhood":
			// for each classifier:
			//     for each neighboring pixel:
			//         for each probability:
			//             if highest, make new highest
			//     add highest label to N
			std::vector<int> selectedNeighbors;
			for (size_t i = 0; i < numberOfInputFiles; ++i)
			{
				double maxProb = 0;
				int label = -1;
				int selectedNeighbor;
				for (size_t l = 1; l < m_labelCount; ++l)
				{
					for (int n = 0; n < probIt[i][l].Size(); ++n)
					{
						bool isInBounds;
						double curProb = probIt[i][l].GetPixel(n, isInBounds);
						if (isInBounds && curProb > maxProb)
						{
							maxProb = curProb;
							label = l;
							selectedNeighbor = n;
						}
					}
				}
				++neiLabelFreq[label];
			}

			// now we have F, S, N -> build list of candidates (up to 3):
			//     label(s) appearing first and second most often in F
			//     label appearing most often in N
			int fgCand = 0, sgCand = -1;
			for (size_t l = 1; l < m_labelCount; ++l)
			{
				if (fbgLabelFreq[l] > fbgLabelFreq[fgCand])
				{
					sgCand = fgCand;
					fgCand = l;
				}
				else if ((sgCand == -1 && fbgLabelFreq[l] > 0) || (sgCand != -1 && fbgLabelFreq[l] > fbgLabelFreq[sgCand]))
				{
					sgCand = l;
				}
			}
			std::vector<int> candidateLabels;
			candidateLabels.push_back(fgCand);
			if (sgCand != -1)
			{
				candidateLabels.push_back(sgCand);
			}

			int neCand = 0;
			for (size_t l = 1; l < m_labelCount; ++l)
			{
				if (neiLabelFreq[l] > neiLabelFreq[neCand])
				{
					neCand = l;
				}
			}
			if (std::find(candidateLabels.begin(), candidateLabels.end(), neCand) == candidateLabels.end())
			{
				candidateLabels.push_back(neCand);
			}

			// out of these candidates, choose the one that:
			//     appears most often both in FSN and FN
			//     AND has the lowest average uncertainty		-> ignore at the moment
			int maxFSNCount = 0; int maxFSNLabel = -1;
			int maxFNCount = 0;  int maxFNLabel = -1;
			double minUncertainty = 1;
			for (int l : candidateLabels)
			{
				int fsnCount = fbgLabelFreq[l] + sbgLabelFreq[l] + neiLabelFreq[l];
				int fnCount = fbgLabelFreq[l] + neiLabelFreq[l];
				/*
				double uncertaintySum;
				for (size_t i = 0; i < numberOfInputFiles; ++i)
				{
					double entropy = 0.0;
					for (unsigned int l = 0; l < m_LabelCount; ++l)
					{

					}
				}
				*/
				if (fsnCount > maxFSNCount)
				{
					maxFSNCount = fsnCount; maxFSNLabel = l;
				}
				if (fnCount > maxFNCount)
				{
					maxFNCount = fnCount; maxFNLabel = l;
				}
			}
			if (maxFNCount != maxFSNCount)
			{
				DEBUG_LOG(QString("Ambiguous result in pixel (%1, %2, %3)")
					.arg(it.GetIndex()[0]).arg(it.GetIndex()[1]).arg(it.GetIndex()[2]));
			}
			out.Set(maxFSNLabel);
		}
		else
		{
			out.Set(it.GetCenterPixel());
		}
		// advance to next pixel in input:
		++it;
		for (size_t i = 0; i < numberOfInputFiles; ++i)
		{
			for (size_t l = 0; l < m_labelCount; ++l)
			{
				++(probIt[i][l]);
			}
		}
		progress.CompletedPixel();
	}
}

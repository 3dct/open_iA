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

#include "iAUndecidedPixelClassifierImageFilter.h"

#include <itkImageRegionIterator.h>
#include <itkConstNeighborhoodIterator.h>
#include <itkMath.h>
#include <itkProgressReporter.h>
#include <itkStatisticsImageFilter.h>

#include "iAConsole.h"
#include "iAMathUtility.h"

#include <QString>

template< typename TInputImage, typename TOutputImage >
iAUndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::iAUndecidedPixelClassifierImageFilter():
	m_undecidedPixelLabel(0),
	m_labelCount(0),
	m_uncertaintyTieSolver(true),
	m_hasUndecidedPixelLabel(false)
{
	m_radius.Fill(1);
}

template< typename TInputImage, typename TOutputImage >
void
iAUndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
	Superclass::PrintSelf(os, indent);
	os << indent << "m_undecidedPixelLabel = " << this->m_undecidedPixelLabel << std::endl;
}

template< typename TInputImage, typename TOutputImage >
typename iAUndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >::InputPixelType
iAUndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::ComputeMaximumInputValue()
{
	InputPixelType maxLabel = 0;
	itk::ImageRegionConstIterator<TInputImage> it(this->GetInput(0), this->GetInput(0)->GetBufferedRegion());
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
iAUndecidedPixelClassifierImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();
	m_labelCount = static_cast<size_t>(this->ComputeMaximumInputValue());
	if (!m_hasUndecidedPixelLabel)
	{
		m_undecidedPixelLabel = static_cast<OutputPixelType>(this->m_labelCount);
	}
	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();
}

template< typename TInputImage, typename TOutputImage >
void iAUndecidedPixelClassifierImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	const size_t numberOfClassifiers = m_probImgs.size();
	if (numberOfClassifiers < 2)
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

	auto output = this->GetOutput();
	
	IntConstNeighborIt it(m_radius, this->GetInput(0), outputRegionForThread);
	it.GoToBegin();
	std::vector<std::vector<DblConstNeighborIt> > probIt;
	for (size_t i = 0; i < numberOfClassifiers; ++i)
	{
		probIt.push_back(std::vector<DblConstNeighborIt>());
		for (size_t l = 0; l < m_labelCount; ++l)
		{
			probIt[i].push_back(DblConstNeighborIt(m_radius, m_probImgs[i][l], outputRegionForThread));
			probIt[i][l].GoToBegin();
		}
	}
	auto out = itk::ImageRegionIterator<TOutputImage>(output, outputRegionForThread);
	double normalizeFactor = 1.0 / -std::log(1.0 / numberOfClassifiers);
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		if (it.GetCenterPixel() == m_undecidedPixelLabel) // only change currently undecided
		{
			std::vector<int> fbgLabelFreq(m_labelCount);
			std::vector<int> sbgLabelFreq(m_labelCount);
			std::vector<int> neiLabelFreq(m_labelCount);
			std::vector<int> fbgLabels(numberOfClassifiers);
			std::vector<int> neiLabels(numberOfClassifiers);
			std::vector<double> curUncertainty(numberOfClassifiers);
			std::vector<double> neiUncertainty(numberOfClassifiers);

			// for first/second best set:
			// for each classifier:
			//     for each probability:
			//         if higher than fbg, make new fbg, move fbg to sbg
			//          else if higher than sbg, make new sbg
			//     add fbg label to F, add sbg label to S
			//     (=increase counter for fbg and sbg label in frequency histograms F and S)

			for (size_t i = 0; i < numberOfClassifiers; ++i)
			{
				int fbgLabel = 0, sbgLabel = -1;
				double entropy = 0.0;
				for (size_t l = 1; l < m_labelCount; ++l)
				{
					double probValue = probIt[i][l].GetCenterPixel();
					if (probValue > 0)
					{
						entropy += (probValue * std::log(probValue));
					}
					if (probValue > probIt[i][fbgLabel].GetCenterPixel())
					{
						sbgLabel = fbgLabel;
						fbgLabel = l;
					}
					else if (sbgLabel == -1 || probValue > probIt[i][sbgLabel].GetCenterPixel())
					{
						sbgLabel = l;
					}
				}
				entropy = clamp(0.0, 1.0, -entropy*normalizeFactor);
				curUncertainty[i] = entropy;
				fbgLabels[i] = fbgLabel;
				fbgLabelFreq[fbgLabel]++;
				sbgLabelFreq[sbgLabel]++;
			}

			// for "best guess of each classifier in neighbourhood":
			// for each classifier:
			//     for each neighboring pixel:
			//         for each probability:
			//             if highest, make new highest
			//     add highest label to N
			std::vector<int> selectedNeighbors;
			for (size_t i = 0; i < numberOfClassifiers; ++i)
			{
				double maxProb = 0;
				int label = -1;
				int selectedNeighbor = -1;
				for (size_t l = 0; l < m_labelCount; ++l)
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
				if (selectedNeighbor == -1)
				{
					DEBUG_LOG("No neighbor found with probability higher than 0!");
				}
				double entropy = 0.0;
				for (size_t l = 0; l < m_labelCount; ++l)
				{
					double probValue = probIt[i][l].GetPixel(selectedNeighbor);
					if (probValue > 0)
					{
						entropy += (probValue * std::log(probValue));
					}
				}
				entropy = clamp(0.0, 1.0, -entropy*normalizeFactor);
				neiLabels[i] = label;
				neiUncertainty[i] = entropy;
				++neiLabelFreq[label];
			}

			// now we have F, S, N -> build list of candidates (up to 3):
			//     label(s) appearing first and second most often in F
			//     label appearing most often in N
			int fgCand = 0, sgCand = -1;
			for (size_t l = 1; l < m_labelCount; ++l)
			{
				// what if another label has the same frequency?
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
				// what if another label has the same frequency?
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
			if (maxFNLabel != maxFSNLabel && m_uncertaintyTieSolver)
			{
				std::vector<double> candLabelUncertaintySum(candidateLabels.size());
				std::vector<int> candLabelUncertaintyCnt(candidateLabels.size());
				std::fill(candLabelUncertaintySum.begin(), candLabelUncertaintySum.end(), 0);
				std::fill(candLabelUncertaintyCnt.begin(), candLabelUncertaintyCnt.end(), 0);
				for (int i = 0; i < numberOfClassifiers; ++i)
				{
					for (int c = 0; c < candidateLabels.size(); ++c)
					{
						if (fbgLabels[i] == candidateLabels[c])
						{
							candLabelUncertaintySum[c] += curUncertainty[i];
							++candLabelUncertaintyCnt[c];
						}
						if (neiLabels[i] == candidateLabels[c])
						{
							candLabelUncertaintySum[c] += neiUncertainty[i];
							++candLabelUncertaintyCnt[c];
						}
					}
				}
				double minUncertainty = 1;
				int finalLabel = -1;
				for (int c = 0; c < candidateLabels.size(); ++c)
				{
					double uncertainty = candLabelUncertaintySum[c] / candLabelUncertaintyCnt[c];
					if (uncertainty < minUncertainty)
					{
						minUncertainty = uncertainty;
						finalLabel = candidateLabels[c];
					}
				}
				/*
				DEBUG_LOG(QString("Ambiguous result in pixel (%1): candidates=%2, prob.=%3, final label=%4")
					.arg(QString("%1, %2, %3")
						.arg(it.GetIndex()[0])
						.arg(it.GetIndex()[1])
						.arg(it.GetIndex()[2]))
					.arg(QString("%1, %2, %3")
						.arg(candidateLabels[0])
						.arg(candidateLabels[1])
						.arg(candidateLabels.size() > 2 ? candidateLabels[2] : -1))
					.arg(QString("%1, %2, %3")
						.arg(candLabelUncertaintySum[0] / candLabelUncertaintyCnt[0])
						.arg(candLabelUncertaintySum[1] / candLabelUncertaintyCnt[1])
						.arg(candidateLabels.size() > 2 ? candLabelUncertaintySum[2] / candLabelUncertaintyCnt[2] : 0))
					.arg(finalLabel));
				*/
				maxFSNLabel = finalLabel;
			}
			out.Set(maxFSNLabel);
		}
		else
		{
			out.Set(it.GetCenterPixel());
		}
		// advance to next pixel in input:
		++it;
		for (size_t i = 0; i < numberOfClassifiers; ++i)
		{
			for (size_t l = 0; l < m_labelCount; ++l)
			{
				++(probIt[i][l]);
			}
		}
		progress.CompletedPixel();
	}
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAProbabilisticVotingImageFilter.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageRegionIterator.h>
#include <itkMath.h>
#include <itkProgressReporter.h>
#include <itkStatisticsImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <iAMathUtility.h>

template< typename TInputImage, typename TOutputImage >
iAProbabilisticVotingImageFilter<TInputImage, TOutputImage>::iAProbabilisticVotingImageFilter():
	m_votingRule(MajorityVoteRule),
	m_labelCount(0),
	m_numberOfClassifiers(0),
	m_undecidedUncertaintyThresh(0.5),
	m_undecidedPixels(0)
{
	
}

template< typename TInputImage, typename TOutputImage >
void iAProbabilisticVotingImageFilter< TInputImage, TOutputImage >
::PrintSelf(std::ostream & /*os*/, itk::Indent /*indent*/) const
{}

template< typename TInputImage, typename TOutputImage >
void iAProbabilisticVotingImageFilter< TInputImage, TOutputImage >
::BeforeThreadedGenerateData()
{
	Superclass::BeforeThreadedGenerateData();
	assert(m_probImgs.size() >= 2);
	m_numberOfClassifiers = m_probImgs.size();
	m_labelCount = m_probImgs[0].size();
	if (m_weights.empty())
	{
		for (size_t c = 0; c < m_numberOfClassifiers; ++c)
		{
			m_weights.push_back(1);
		}
	}

	assert(m_weights.size() == m_numberOfClassifiers);

	typename TOutputImage::Pointer output = this->GetOutput();
	output->SetBufferedRegion(output->GetRequestedRegion());
	output->Allocate();
	itk::SmartPointer<const TInputImage> in0 = this->GetInput(0);
}

template< typename TInputImage, typename TOutputImage >
void iAProbabilisticVotingImageFilter<TInputImage, TOutputImage>::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
	itk::ThreadIdType threadId)
{
	itk::ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

	//typedef itk::ImageRegionConstIterator< TInputImage > IteratorType;
	typedef itk::ImageRegionIterator< TOutputImage >     OutIteratorType;
	//typedef itk::ImageRegionIterator<DoubleImg> DoubleOutIteratorType;

	typename TOutputImage::Pointer output = this->GetOutput();

	std::vector<std::vector<ConstDblIt> > probIt;
	for (size_t c = 0; c < m_numberOfClassifiers; ++c)
	{
		probIt.push_back(std::vector<ConstDblIt>());
		for (size_t l = 0; l < m_labelCount; ++l)
		{
			probIt[c].push_back(ConstDblIt(m_probImgs[c][l], outputRegionForThread));
			probIt[c][l].GoToBegin();
		}
	}

	OutIteratorType out = OutIteratorType(output, outputRegionForThread);

	double * combinedPixelProbs = new double[m_labelCount];
	double normalizeFactor = 1.0 / -std::log(1.0 / m_numberOfClassifiers);
	for (out.GoToBegin(); !out.IsAtEnd(); ++out)
	{
		double normalizationSum = 0;
		std::fill(combinedPixelProbs, combinedPixelProbs + m_labelCount, 0);
		for (size_t l = 0; l < m_labelCount; ++l)
		{
			switch (m_votingRule)
			{
			case SumRule:
				for (size_t c = 0; c < m_numberOfClassifiers; ++c)
				{
					combinedPixelProbs[l] += probIt[c][l].Get();
				}
				combinedPixelProbs[l] /= m_numberOfClassifiers;
				break;
			case MaxRule:
				for (size_t c = 0; c < m_numberOfClassifiers; ++c)
				{
					combinedPixelProbs[l] = std::max(probIt[c][l].Get(), combinedPixelProbs[l]);
				}
				break;
			case MinRule: {
				double curLabelProb = 1;
				for (size_t c = 0; c < m_numberOfClassifiers; ++c)
				{
					curLabelProb = std::min(probIt[c][l].Get(), curLabelProb);
				}
				combinedPixelProbs[l] = curLabelProb;
				break;
			}
			case MedianRule: {
				std::vector<double> probs;
				for (size_t c = 0; c < m_numberOfClassifiers; ++c)
				{
					probs.push_back(probIt[c][l].Get());
				}
				std::sort(probs.begin(), probs.end());
				combinedPixelProbs[l] = probs.size() % 2 == 0
					? (probs[(probs.size() / 2) - 1] + probs[probs.size() / 2]) / 2
					: probs[probs.size() / 2];
				break;
			}
			case MajorityVoteRule: {
				double majorityCount = 0;
				for (size_t c = 0; c < m_numberOfClassifiers; ++c)
				{
					bool isMajority = true;
					for (size_t l2 = 0; l2 < m_labelCount; ++l2)
					{
						if (l == l2)
						{
							continue;
						}
						if (probIt[c][l2].Get() > probIt[c][l].Get() ||
							(l2 < l && probIt[c][l2].Get() == probIt[c][l].Get()))
							// if two highest probabilities are the same, only count it
							// as a vote towards the lower label index (to not count doubles)
						{
							isMajority = false;
							break;
						}
					}
					if (isMajority)
					{
						majorityCount += m_weights[c];
					}
				}
				combinedPixelProbs[l] = majorityCount / m_numberOfClassifiers;
				break;
			}
			}
			normalizationSum += combinedPixelProbs[l];
		}

		// determine max probability:
		/*
		if (std::abs(normalizationSum - 1.0) > std::numeric_limits<float>::epsilon())
		{
			LOG(lvlWarn, "Normalization Sum != 1!");
		}
		*/
		size_t maxProbIdx = 0;
		double entropy = 0.0;
		for (size_t l = 0; l < m_labelCount; ++l)
		{
			combinedPixelProbs[l] /= normalizationSum;
			if (combinedPixelProbs[l] > combinedPixelProbs[maxProbIdx])
			{
				maxProbIdx = l;
			}
			if (combinedPixelProbs[l] > 0)
			{
				entropy += (combinedPixelProbs[l] * std::log(combinedPixelProbs[l]));
			}
		}
		entropy = clamp(0.0, 1.0, -entropy*normalizeFactor);

		size_t finalLabel = maxProbIdx;
		if (entropy >= m_undecidedUncertaintyThresh)
		{
			finalLabel = m_labelCount;
			m_undecidedPixels += 1;
		}

		// with probabilities, set output (TODO: also output probabilities?)
		out.Set(static_cast<int>(finalLabel));

		// advance to next pixel:
		for (size_t c = 0; c < m_numberOfClassifiers; ++c)
		{
			for (size_t l = 0; l < m_labelCount; ++l)
			{
				++(probIt[c][l]);
			}
		}
		progress.CompletedPixel();

	}
	delete[] combinedPixelProbs;
}

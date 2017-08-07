/*************************************  open_iA  ************************************ *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "iAitkRandomWalker.h"

#include "iAImageCoordinate.h"
#include "iANormalizerImpl.h"
#include "iARandomWalker.h"
#include "iAVectorArrayImpl.h"
#include "iAVectorDistanceImpl.h"


template <class TInputImage>
QSharedPointer<iAVectorArray> GetPixelValueListFromImage(TInputImage* input)
{
	assert(input);

	typename TInputImage::RegionType inputRegion = input->GetLargestPossibleRegion();
	typename TInputImage::SizeType inputSize = inputRegion.GetSize();

	int numOfEntries = inputSize[0]*inputSize[1]*inputSize[2];
	int nrOfComponents = 1;
	QSharedPointer<iAitkPixelVectorArray<TInputImage> > data(new
		iAitkPixelVectorArray<TInputImage>
		(inputSize[0], inputSize[1], inputSize[2]));
	data->AddImage(input);
	return data;
}



// iAitkRandomWalker

template <class TInputImage>
iAitkRandomWalker<TInputImage>::iAitkRandomWalker()
{}

template <class TInputImage>
iAitkRandomWalker<TInputImage>::~iAitkRandomWalker()
{
}

template <class TInputImage>
void iAitkRandomWalker<TInputImage>::Calculate()
{
	typename TInputImage::RegionType inputRegion = m_input->GetLargestPossibleRegion();
	typename TInputImage::SizeType inputSize = inputRegion.GetSize();

	iARWInputChannel input1;

	input1.image = GetPixelValueListFromImage(m_input);

	// at the moment, hardcode distance function and normalizer
	input1.distanceFunc = QSharedPointer<iAVectorDistance>(new iASquaredDistance());
	iAGaussianNormalizer* n = new iAGaussianNormalizer();
	n->SetBeta(m_beta);
	input1.normalizeFunc = QSharedPointer<iANormalizer>(n); // m_normalizeFunc;
	input1.weight = 1;
	QSharedPointer<QVector<iARWInputChannel> > inputChannels(new QVector<iARWInputChannel>());
	inputChannels->push_back(input1);
	
	typename TInputImage::SpacingType itkSpacing = m_input->GetSpacing();
	double spacing[3];
	for (int i=0; i<3; ++i) spacing[i] = itkSpacing[i];

	m_randomWalker = QSharedPointer<iARandomWalker>(new iARandomWalker(
		inputSize[0], inputSize[1], inputSize[2],
		spacing,
		inputChannels,
		m_seeds
	));
	m_randomWalker->start();
	m_randomWalker->wait();
	m_result = m_randomWalker->GetResult();
}

template <class TInputImage>
QVector<iAITKIO::ImagePointer> iAitkRandomWalker<TInputImage>::GetProbabilityImages()
{
	assert(m_result);
	return m_result->probabilityImages;
}

template <class TInputImage>
LabelImagePointer iAitkRandomWalker<TInputImage>::GetLabelImage()
{
	assert(m_result);
	// TODO: might cause memory leak!
	return dynamic_cast<LabelImageType*>(m_result->labelledImage.GetPointer());
}

template <class TInputImage>
void iAitkRandomWalker<TInputImage>::SetInput(TInputImage* image, SeedVector seeds, double beta)
{
	m_input = image;
	m_seeds = seeds;
	m_beta = beta;
}

template <class TInputImage>
bool iAitkRandomWalker<TInputImage>::Success() const
{
	return m_result;
}



// iAitkExtendedRandomWalker

template <class TInputImage>
iAitkExtendedRandomWalker<TInputImage>::iAitkExtendedRandomWalker():
	m_priorModel(new QVector<PriorModelImagePointer>())
{
}

template <class TInputImage>
iAitkExtendedRandomWalker<TInputImage>::~iAitkExtendedRandomWalker()
{
}

template <class TInputImage>
void iAitkExtendedRandomWalker<TInputImage>::Calculate()
{
	typename TInputImage::RegionType inputRegion = m_input->GetLargestPossibleRegion();
	typename TInputImage::SizeType inputSize = inputRegion.GetSize();

	iARWInputChannel input1;

	input1.image = GetPixelValueListFromImage(m_input);
	input1.distanceFunc = QSharedPointer<iAVectorDistance>(new iASquaredDistance());// m_distanceFuncs[0];
	iAGaussianNormalizer* n = new iAGaussianNormalizer();
	n->SetBeta(1.0);
	input1.normalizeFunc = QSharedPointer<iANormalizer>(n); // m_normalizeFunc;
	input1.weight = 1;
	QSharedPointer<QVector<iARWInputChannel> > inputChannels(new QVector<iARWInputChannel>());
	inputChannels->push_back(input1);

	double gamma = 1.0;
	int maxIter = 50;

	typename TInputImage::SpacingType itkSpacing = m_input->GetSpacing();
	double spacing[3];
	for (int i=0; i<3; ++i) spacing[i] = itkSpacing[i];

	m_extendedRandomWalker = QSharedPointer<iAExtendedRandomWalker>(new iAExtendedRandomWalker(
		inputSize[0], inputSize[1], inputSize[2],
		spacing,
		inputChannels,
		m_priorModel,
		gamma,
		maxIter
	));
	m_extendedRandomWalker->start();
	m_extendedRandomWalker->wait();
	m_result = m_extendedRandomWalker->GetResult();
}

template <class TInputImage>
QVector<iAITKIO::ImagePointer> iAitkExtendedRandomWalker<TInputImage>::GetProbabilityImages()
{
	assert(m_result);
	return m_result->probabilityImages;
}

template <class TInputImage>
LabelImagePointer iAitkExtendedRandomWalker<TInputImage>::GetLabelImage()
{
	assert(m_result);
	// TODO: might cause memory leak!
	return dynamic_cast<LabelImageType*>(m_result->labelledImage.GetPointer());
}

template <class TInputImage>
void iAitkExtendedRandomWalker<TInputImage>::AddPriorModel(PriorModelImagePointer priorModel)
{
	m_priorModel->push_back(priorModel);
}

template <class TInputImage>
void iAitkExtendedRandomWalker<TInputImage>::SetInput(TInputImage* image)
{
	m_input = image;
}

template <class TInputImage>
bool iAitkExtendedRandomWalker<TInputImage>::Success() const
{
	return m_result;
}

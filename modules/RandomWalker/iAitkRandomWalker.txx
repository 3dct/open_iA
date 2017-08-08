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
//#pragma once

#include "iAitkRandomWalker.h"

#include "iAImageCoordinate.h"
#include "iANormalizerImpl.h"
#include "iAVectorArrayImpl.h"
#include "iAVectorDistanceImpl.h"


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
	m_randomWalker = QSharedPointer<iARandomWalker>(new iARandomWalker(
		m_size[0], m_size[1], m_size[2],
		m_spacing,
		m_inputChannels,
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
void iAitkRandomWalker<TInputImage>::SetInput(QSharedPointer<QVector<iARWInputChannel> > input)
{
	m_inputChannels = input;
}

template <class TInputImage>
void iAitkRandomWalker<TInputImage>::SetParams(int maxIter, int const size[3], double const spacing[3], QSharedPointer<iASeedVector> seeds)
{
	m_maxIter = maxIter;
	std::copy(size, size + 3, m_size);
	std::copy(spacing, spacing + 3, m_spacing);
	m_seeds = seeds;
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
	m_extendedRandomWalker = QSharedPointer<iAExtendedRandomWalker>(new iAExtendedRandomWalker(
		m_size[0], m_size[1], m_size[2],
		m_spacing,
		m_inputChannels,
		m_priorModel,
		m_gamma,
		m_maxIter
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
void iAitkExtendedRandomWalker<TInputImage>::SetInput(QSharedPointer<QVector<iARWInputChannel> > input)
{
	m_inputChannels = input;
}

template <class TInputImage>
bool iAitkExtendedRandomWalker<TInputImage>::Success() const
{
	return m_result;
}

template <class TInputImage>
void iAitkExtendedRandomWalker<TInputImage>::SetParams(int maxIter, int const size[3], double const spacing[3],
	QSharedPointer<QVector<PriorModelImagePointer> > priorModel, double gamma)
{
	m_maxIter = maxIter;
	std::copy(size, size + 3, m_size);
	std::copy(spacing, spacing + 3, m_spacing);
	m_priorModel = priorModel;
	m_gamma = gamma;
}

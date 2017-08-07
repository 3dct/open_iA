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

#include "iARandomWalker.h"

// wrapper classes to make the (extended) random walker applicable for arbitrary itk imges

template <class TInputImage>
class iAitkRandomWalker
{
public:
	typedef iAitkRandomWalker				  Self;
	iAitkRandomWalker();
	~iAitkRandomWalker();
	void SetInput(QSharedPointer<QVector<iARWInputChannel> > input);
	void SetParams(int maxIter, int const size[3], double const  spacing[3], QSharedPointer<SeedVector> seeds);

	bool Success() const;
	LabelImagePointer GetLabelImage();
	QVector<iAITKIO::ImagePointer> GetProbabilityImages();
	void Calculate();
private:
	// not implemented on purpose:
	iAitkRandomWalker(const Self &);
	void operator=(const Self &);

	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	QSharedPointer<SeedVector> m_seeds;
	int m_maxIter;
	int m_size[3];
	double m_spacing[3];

	QSharedPointer<iARandomWalker> m_randomWalker;
	QSharedPointer<iARWResult> m_result;
};

class iAExtendedRandomWalker;

template <class TInputImage>
class iAitkExtendedRandomWalker
{
public:
	typedef iAitkExtendedRandomWalker				  Self;
	iAitkExtendedRandomWalker();
	~iAitkExtendedRandomWalker();
	void SetInput(QSharedPointer<QVector<iARWInputChannel> > input);
	void SetParams(int maxIter, int const size[3], double const spacing[3], QSharedPointer<QVector<PriorModelImagePointer> > priors, double gamma);

	bool Success() const;
	LabelImagePointer GetLabelImage();
	QVector<iAITKIO::ImagePointer> GetProbabilityImages();
	void Calculate();
private:
	// not implemented on purpose:
	iAitkExtendedRandomWalker(const Self &);
	void operator=(const Self &);
	
	QSharedPointer<QVector<iARWInputChannel> > m_inputChannels;
	QSharedPointer<QVector<PriorModelImagePointer> > m_priorModel;
	double m_gamma;
	int m_maxIter;
	int m_size[3];
	double m_spacing[3];

	QSharedPointer<iAExtendedRandomWalker> m_extendedRandomWalker;
	QSharedPointer<iARWResult> m_result;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAitkRandomWalker.txx"
#endif

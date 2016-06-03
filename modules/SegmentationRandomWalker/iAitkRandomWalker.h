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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IA_ITK_RANDOM_WALKER
#define IA_ITK_RANDOM_WALKER

#include "iARandomWalker.h"

// wrapper classes to make the (extended) random walker applicable for arbitrary itk imges

template <class TInputImage>
class iAitkRandomWalker
{
public:
	typedef iAitkRandomWalker				  Self;
	iAitkRandomWalker();
	~iAitkRandomWalker();
	void SetInput(TInputImage* image, SeedVector, double beta);
	bool Success() const;
	LabelImagePointer GetLabelImage();
	QVector<ProbabilityImagePointer> GetProbabilityImages();
	void Calculate();
private:
	// not implemented on purpose:
	iAitkRandomWalker(const Self &);
	void operator=(const Self &);
	
	QSharedPointer<iARandomWalker> m_randomWalker;
	
	TInputImage* m_input;
	SeedVector m_seeds;
	double m_beta;
	QSharedPointer<iARWResult> m_result;
};

template <class TInputImage>
class iAitkExtendedRandomWalker
{
public:
	typedef iAitkExtendedRandomWalker				  Self;
	iAitkExtendedRandomWalker();
	~iAitkExtendedRandomWalker();
	void SetInput(TInputImage* image);
	void AddPriorModel(PriorModelImagePointer priorModel);
	bool Success() const;
	LabelImagePointer GetLabelImage();
	QVector<ProbabilityImagePointer> GetProbabilityImages();
	void Calculate(); 
private:
	// not implemented on purpose:
	iAitkExtendedRandomWalker(const Self &);
	void operator=(const Self &);
	
	QSharedPointer<iAExtendedRandomWalker> m_extendedRandomWalker;
	
	TInputImage* m_input;
	QSharedPointer<QVector<PriorModelImagePointer> > m_priorModel;
	QSharedPointer<iARWResult> m_result;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAitkRandomWalker.hxx"
#endif

#endif
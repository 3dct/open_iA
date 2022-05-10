/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iAFilter.h>

#include <iAItkVersion.h>

#if (!defined(ITKNOGPU) && (ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5,1,0) && ITK_VERSION_NUMBER < ITK_VERSION_CHECK(5,2,0)))
#ifndef _MSC_VER
#warning("With ITK 5.1.x, GPU-accelerated filters don't work in open_iA, see https://github.com/InsightSoftwareConsortium/ITK/issues/1381. Disabling GPU support")
#else
#pragma message("With ITK 5.1.x, GPU-accelerated filters don't work in open_iA, see https://github.com/InsightSoftwareConsortium/ITK/issues/1381. Disabling GPU support")
#endif
#define ITKNOGPU
#endif

namespace itk
{
	class ProcessObject;
}

// Blurring
IAFILTER_DEFAULT_CLASS(iADiscreteGaussian);
IAFILTER_DEFAULT_CLASS(iARecursiveGaussian);
IAFILTER_DEFAULT_CLASS(iAMedianFilter);
class iANonLocalMeans : public iAFilter
{
public:
	static QSharedPointer<iANonLocalMeans> create();
	void abort() override;
private:
	void performWork(QMap<QString, QVariant> const& parameters) override;
	iANonLocalMeans();
	itk::ProcessObject * m_itkProcess;
};

// Edge-Preserving
IAFILTER_DEFAULT_CLASS(iAGradientAnisotropicDiffusion);
IAFILTER_DEFAULT_CLASS(iACurvatureAnisotropicDiffusion);
IAFILTER_DEFAULT_CLASS(iACurvatureFlow);
IAFILTER_DEFAULT_CLASS(iABilateral);
#ifndef ITKNOGPU
IAFILTER_DEFAULT_CLASS(iAGPUEdgePreservingSmoothing)
#endif

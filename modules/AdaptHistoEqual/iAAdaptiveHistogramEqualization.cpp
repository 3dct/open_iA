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
#include "pch.h"
#include "iAAdaptiveHistogramEqualization.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkAdaptiveHistogramEqualizationImageFilter.h>

template<class T> void iAAdaptiveHistogramEqualization_template( double alpha, double beta, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef  itk::AdaptiveHistogramEqualizationImageFilter< InputImageType > AdaptiveHistogramEqualizationImageFilterType;
	auto castImage = dynamic_cast< InputImageType * >( image->GetITKImage() );
	auto adaptiveHistogramEqualizationImageFilter = AdaptiveHistogramEqualizationImageFilterType::New();
	adaptiveHistogramEqualizationImageFilter->SetInput( castImage );
	adaptiveHistogramEqualizationImageFilter->SetAlpha( alpha );
	adaptiveHistogramEqualizationImageFilter->SetBeta( beta );
	adaptiveHistogramEqualizationImageFilter->SetRadius( 1 );
	p->Observe( adaptiveHistogramEqualizationImageFilter );
	adaptiveHistogramEqualizationImageFilter->Update();
	image->SetImage(adaptiveHistogramEqualizationImageFilter->GetOutput());
	image->Modified();
	adaptiveHistogramEqualizationImageFilter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAAdaptiveHistogramEqualization)

void iAAdaptiveHistogramEqualization::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(iAAdaptiveHistogramEqualization_template, pixelType,
		parameters["Alpha"].toDouble(),
		parameters["Beta"].toDouble(),
		m_progress, m_con);
}

iAAdaptiveHistogramEqualization::iAAdaptiveHistogramEqualization() :
	iAFilter("Adaptive Histogram Equalization", "",
		"This filter is a superset of many contrast enhancing filters.<br/>"
		"By modifying its parameters (alpha, beta), the filter can produce an "
		"adaptively equalized histogram or a version of unsharp mask (local "
		"mean subtraction).<br/>"
		"The parameter alpha controls how much the filter acts like the "
		"classical histogram equalization method (alpha=0) to how much the "
		"filter acts like an unsharp mask (alpha=1). The parameter beta "
		"controls how much the filter acts like an unsharp mask (beta=0) to "
		"how much the filter acts like pass through (beta=1, with alpha=1)."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AdaptiveHistogramEqualizationImageFilter.html\">"
		"Adaptive Histogram Equalization Filter</a> in the ITK documentation.")
{
	AddParameter("Alpha", Continuous, 0, 0, 1);
	AddParameter("Beta", Continuous, 0, 0, 1);
}

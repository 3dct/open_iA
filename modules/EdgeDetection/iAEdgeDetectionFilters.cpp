/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAEdgeDetectionFilters.h"

#include <iAConnector.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkCannyEdgeDetectionImageFilter.h>
#include <itkCastImageFilter.h>

template<class T>
void canny_edge_detection(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;
	typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	typedef itk::CannyEdgeDetectionImageFilter < RealImageType, RealImageType > CannyEDFType;

	auto toReal = CastToRealFilterType::New();
	toReal->SetInput( dynamic_cast< InputImageType * >( filter->Input()[0]->GetITKImage() ) );
	auto canny = CannyEDFType::New();
	canny->SetVariance(parameters["Variance"].toDouble());
	canny->SetMaximumError(parameters["Maximum error"].toDouble());
	canny->SetUpperThreshold(parameters["Upper threshold"].toDouble());
	canny->SetLowerThreshold(parameters["Lower threshold"].toDouble());
	canny->SetInput( toReal->GetOutput() );
	filter->Progress()->Observe( canny );
	canny->Update();
	filter->AddOutput(canny->GetOutput());
}

IAFILTER_CREATE(iACannyEdgeDetection)

void iACannyEdgeDetection::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(canny_edge_detection, InputPixelType(), this, parameters);
}

iACannyEdgeDetection::iACannyEdgeDetection() :
	iAFilter("Canny", "Edge detection",
		"Canny edge detector for scalar-valued images.<br/>"
		"<em>Variance</em> and <em>Maximum error</em> parameters are used to perform "
		"a Gaussian smoothing of the image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CannyEdgeDetectionImageFilter.html\">"
		"Canny Edge Detection Filter</a> in the ITK documentation.")
{
	AddParameter("Variance", Continuous, 0.01);
	AddParameter("Maximum error", Continuous, 0.01, std::numeric_limits<double>::epsilon(), 1);
	AddParameter("Lower threshold", Continuous, 0);
	AddParameter("Upper threshold", Continuous, 1);
}

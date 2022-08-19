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
#include "iAEdgeDetectionFilters.h"

#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkCannyEdgeDetectionImageFilter.h>
#include <itkSobelEdgeDetectionImageFilter.h>

namespace
{
	using RealPixelType = float;
	using RealImageType = itk::Image<RealPixelType, 3>;
}

template<class T>
void canny_edge_detection(iAFilter* filter, QVariantMap const & parameters)
{
	using EdgeDetectionType = itk::CannyEdgeDetectionImageFilter<RealImageType, RealImageType>;

	auto inImg = castImageTo<RealPixelType>(filter->input(0)->itkImage());
	auto canny = EdgeDetectionType::New();
	canny->SetVariance(parameters["Variance"].toDouble());
	canny->SetMaximumError(parameters["Maximum error"].toDouble());
	canny->SetUpperThreshold(parameters["Upper threshold"].toDouble());
	canny->SetLowerThreshold(parameters["Lower threshold"].toDouble());
	canny->SetInput(dynamic_cast<RealImageType*>(inImg.GetPointer()));
	filter->progress()->observe( canny );
	canny->Update();
	filter->addOutput(canny->GetOutput());
}

IAFILTER_CREATE(iACannyEdgeDetection)

void iACannyEdgeDetection::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(canny_edge_detection, inputPixelType(), this, parameters);
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
	addParameter("Variance", iAValueType::Continuous, 0.01);
	addParameter("Maximum error", iAValueType::Continuous, 0.01, std::numeric_limits<double>::epsilon(), 1);
	addParameter("Lower threshold", iAValueType::Continuous, 0);
	addParameter("Upper threshold", iAValueType::Continuous, 1);
}



template <class T>
void sobel_edge_detection(iAFilter* filter, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);
	using EdgeDetectionType = itk::SobelEdgeDetectionImageFilter<RealImageType, RealImageType> ;

	auto inImg = castImageTo<RealPixelType>(filter->input(0)->itkImage());
	auto edgeDetector = EdgeDetectionType::New();
	edgeDetector->SetInput(dynamic_cast<RealImageType*>(inImg.GetPointer()));
	filter->progress()->observe(edgeDetector);
	edgeDetector->Update();
	filter->addOutput(edgeDetector->GetOutput());
}

IAFILTER_CREATE(iASobelEdgeDetection)

void iASobelEdgeDetection::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(sobel_edge_detection, inputPixelType(), this, parameters);
}

iASobelEdgeDetection::iASobelEdgeDetection() :
	iAFilter("Sobel", "Edge detection",
		"Sobel edge detector for scalar-valued images.<br/>"
		"Uses the Sobel operator to calculate the image gradient and then finds the magnitude of this gradient vector."
		" The Sobel gradient magnitude (square-root sum of squares) is an indication of edge strength.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SobelEdgeDetectionImageFilter.html\">"
		"Sobel Edge Detection Filter</a> in the ITK documentation.")
{
}

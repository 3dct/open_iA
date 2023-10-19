// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkCannyEdgeDetectionImageFilter.h>
#include <itkSobelEdgeDetectionImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

IAFILTER_DEFAULT_CLASS(iACannyEdgeDetection);
IAFILTER_DEFAULT_CLASS(iASobelEdgeDetection);

namespace
{
	using RealPixelType = float;
	using RealImageType = itk::Image<RealPixelType, 3>;
}

template<class T>
void canny_edge_detection(iAFilter* filter, QVariantMap const & parameters)
{
	using EdgeDetectionType = itk::CannyEdgeDetectionImageFilter<RealImageType, RealImageType>;

	auto inImg = castImageTo<RealPixelType>(filter->imageInput(0)->itkImage());
	auto canny = EdgeDetectionType::New();
	canny->SetVariance(parameters["Variance"].toDouble());
	canny->SetMaximumError(parameters["Maximum error"].toDouble());
	canny->SetUpperThreshold(parameters["Upper threshold"].toDouble());
	canny->SetLowerThreshold(parameters["Lower threshold"].toDouble());
	canny->SetInput(dynamic_cast<RealImageType*>(inImg.GetPointer()));
	filter->progress()->observe( canny );
	canny->Update();
	filter->addOutput(std::make_shared<iAImageData>(canny->GetOutput()));
}

void iACannyEdgeDetection::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(canny_edge_detection, inputScalarType(), this, parameters);
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

	auto inImg = castImageTo<RealPixelType>(filter->imageInput(0)->itkImage());
	auto edgeDetector = EdgeDetectionType::New();
	edgeDetector->SetInput(dynamic_cast<RealImageType*>(inImg.GetPointer()));
	filter->progress()->observe(edgeDetector);
	edgeDetector->Update();
	filter->addOutput(std::make_shared<iAImageData>(edgeDetector->GetOutput()));
}

void iASobelEdgeDetection::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(sobel_edge_detection, inputScalarType(), this, parameters);
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

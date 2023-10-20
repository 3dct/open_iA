// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <defines.h>    // for DIM
#include <iAImageData.h>
#include <iAFilterDefault.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkLaplacianSegmentationLevelSetImageFilter.h>
#include <itkCannySegmentationLevelSetImageFilter.h>
#include <itkZeroCrossingImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

IAFILTER_DEFAULT_CLASS(iACannySegmentationLevelSet);
IAFILTER_DEFAULT_CLASS(iALaplacianSegmentationLevelSet);
IAFILTER_DEFAULT_CLASS(iAZeroCrossing);

template<class T>
void laplacianSegmentationLevelSet(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using FeatureImageType = itk::Image<T, DIM>;
	using OutputPixelType = float;
	using LevelSetFilter = itk::LaplacianSegmentationLevelSetImageFilter<InputImageType, FeatureImageType, OutputPixelType>;
	auto laplacianSegmentation = LevelSetFilter::New();
	laplacianSegmentation->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	laplacianSegmentation->SetFeatureImage(dynamic_cast<FeatureImageType*>(filter->imageInput(1)->itkImage()));
	laplacianSegmentation->SetMaximumRMSError(parameters["Maximum RMS error"].toDouble());
	laplacianSegmentation->SetNumberOfIterations(parameters["Number of iterations"].toULongLong());
	laplacianSegmentation->SetAdvectionScaling(parameters["Advection scaling"].toDouble());
	laplacianSegmentation->SetPropagationScaling(parameters["Propagation scaling"].toDouble());
	laplacianSegmentation->SetCurvatureScaling(parameters["Curvature scaling"].toDouble());
	laplacianSegmentation->SetIsoSurfaceValue(parameters["Iso surface value"].toDouble());
	laplacianSegmentation->SetReverseExpansionDirection(parameters["Reverse expansion direction"].toBool());
	filter->progress()->observe(laplacianSegmentation);
	laplacianSegmentation->Update();
	filter->addOutput(std::make_shared<iAImageData>(laplacianSegmentation->GetOutput()));
}

iALaplacianSegmentationLevelSet::iALaplacianSegmentationLevelSet() :
	iAFilter("Laplacian Segmentation", "Segmentation/Level Set",
		"Segments structures in images based on a second derivative image features.<br/>"
		"The main input image should be a 'seed image', i.e. an image containing an iso surface, "
		"for example in the form of a binary labellebed image. The additional input 1 is the 'feature image',"
		"typically the image you want to segment.<br/>"
		"For more information on this and on the parameters, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1LaplacianSegmentationLevelSetImageFilter.html#details\">"
		"Laplacian Segmentation Level Set Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Maximum RMS error", iAValueType::Continuous, 0.002, 0.0, 1.0);
	addParameter("Number of iterations", iAValueType::Discrete, 1000, 1, static_cast<double>(std::numeric_limits<uint64_t>::max()));
	addParameter("Advection scaling", iAValueType::Continuous, 1.0);
	addParameter("Propagation scaling", iAValueType::Continuous, 1.0);
	addParameter("Curvature scaling", iAValueType::Continuous, 1.0);
	addParameter("Iso surface value", iAValueType::Continuous, 0.5);
	addParameter("Reverse expansion direction", iAValueType::Boolean, false);
}

void iALaplacianSegmentationLevelSet::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(laplacianSegmentationLevelSet, inputScalarType(), this, parameters);
}



template<class T>
void cannySegmentationLevelSet(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using FeatureImageType = itk::Image<T, DIM>;
	using OutputPixelType = float;
	using LevelSetFilter = itk::CannySegmentationLevelSetImageFilter<InputImageType, FeatureImageType, OutputPixelType>;
	auto cannySegmentation = LevelSetFilter::New();
	cannySegmentation->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	cannySegmentation->SetFeatureImage(dynamic_cast<FeatureImageType*>(filter->imageInput(1)->itkImage()));
	cannySegmentation->SetMaximumRMSError(parameters["Maximum RMS error"].toDouble());
	cannySegmentation->SetNumberOfIterations(parameters["Number of iterations"].toULongLong());
	cannySegmentation->SetAdvectionScaling(parameters["Advection scaling"].toDouble());
	cannySegmentation->SetPropagationScaling(parameters["Propagation scaling"].toDouble());
	cannySegmentation->SetCurvatureScaling(parameters["Curvature scaling"].toDouble());
	cannySegmentation->SetIsoSurfaceValue(parameters["Iso surface value"].toDouble());
	cannySegmentation->SetReverseExpansionDirection(parameters["Reverse expansion direction"].toBool());
	filter->progress()->observe(cannySegmentation);
	cannySegmentation->Update();
	filter->addOutput(std::make_shared<iAImageData>(cannySegmentation->GetOutput()));
}

iACannySegmentationLevelSet::iACannySegmentationLevelSet() :
	iAFilter("Canny Segmentation", "Segmentation/Level Set",
		"Segments structures in images based on image features derived from pseudo-canny-edges.<br/>"
		"The main input image should be a 'seed image', i.e. an image containing an iso surface, "
		"for example in the form of a binary labellebed image. The additional input 1 is the 'feature image',"
		"typically the image you want to segment.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CannySegmentationLevelSetImageFilter.html#details\">"
		"Canny Segmentation Level Set Image Filter</a> in the ITK documentation.", 2)
{
	addParameter("Maximum RMS error", iAValueType::Continuous, 0.002, 0.0, 1.0);
	addParameter("Number of iterations", iAValueType::Discrete, 1000, 1, static_cast<double>(std::numeric_limits<uint64_t>::max()));
	addParameter("Advection scaling", iAValueType::Continuous, 1.0);
	addParameter("Propagation scaling", iAValueType::Continuous, 1.0);
	addParameter("Curvature scaling", iAValueType::Continuous, 1.0);
	addParameter("Iso surface value", iAValueType::Continuous, 0.5);
	addParameter("Reverse expansion direction", iAValueType::Boolean, false);
}

void iACannySegmentationLevelSet::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(cannySegmentationLevelSet, inputScalarType(), this, parameters);
}



template<class T>
void zeroCrossing(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using ZeroCrossingFilter = itk::ZeroCrossingImageFilter<InputImageType, InputImageType>;
	auto zeroCrossingFilter = ZeroCrossingFilter::New();
	zeroCrossingFilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	zeroCrossingFilter->SetForegroundValue(parameters["Foreground value"].toDouble());
	zeroCrossingFilter->SetBackgroundValue(parameters["Background value"].toDouble());
	filter->progress()->observe(zeroCrossingFilter);
	zeroCrossingFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(zeroCrossingFilter->GetOutput()));
}

iAZeroCrossing::iAZeroCrossing() :
	iAFilter("Zero Crossing", "Segmentation/Level Set",
		"Finds the closest pixel to the zero-crossings (i.e., sign changes) in a given image.<br/>"
		"Pixels closest to zero-crossings are labeled with a foreground value, all other with a background value."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ZeroCrossingImageFilter.html#details\">"
		"Zero Crossing Image Filter</a> in the ITK documentation.")
{
	addParameter("Background value", iAValueType::Continuous, 0);
	addParameter("Foreground value", iAValueType::Continuous, 1);
}

void iAZeroCrossing::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(zeroCrossing, inputScalarType(), this, parameters);
}

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iALevelSet.h"

#include <itkLaplacianSegmentationLevelSetImageFilter.h>
#include <itkCannySegmentationLevelSetImageFilter.h>
#include <itkZeroCrossingImageFilter.h>

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

template<class T>
void laplacianSegmentationLevelSet(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using FeatureImageType = itk::Image<T, DIM>;
	using OutputPixelType = float;
	using LevelSetFilter = itk::LaplacianSegmentationLevelSetImageFilter<InputImageType, FeatureImageType, OutputPixelType>;
	auto laplacianSegmentation = LevelSetFilter::New();
	laplacianSegmentation->SetInput(dynamic_cast<InputImageType*>(filter->input()[0]->itkImage()));
	laplacianSegmentation->SetFeatureImage(dynamic_cast<FeatureImageType*>(filter->input()[1]->itkImage()));
	laplacianSegmentation->SetMaximumRMSError(parameters["Maximum RMS error"].toDouble());
	laplacianSegmentation->SetNumberOfIterations(parameters["Number of iterations"].toULongLong());
	laplacianSegmentation->SetAdvectionScaling(parameters["Advection scaling"].toDouble());
	laplacianSegmentation->SetPropagationScaling(parameters["Propagation scaling"].toDouble());
	laplacianSegmentation->SetCurvatureScaling(parameters["Curvature scaling"].toDouble());
	laplacianSegmentation->SetIsoSurfaceValue(parameters["Iso surface value"].toDouble());
	laplacianSegmentation->SetReverseExpansionDirection(parameters["Reverse expansion direction"].toBool());
	filter->progress()->observe(laplacianSegmentation);
	laplacianSegmentation->Update();
	filter->addOutput(laplacianSegmentation->GetOutput());
}

IAFILTER_CREATE(iALaplacianSegmentationLevelSet)

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
	addParameter("Maximum RMS error", Continuous, 0.002, 0.0, 1.0);
	addParameter("Number of iterations", Discrete, 1000, 1, std::numeric_limits<unsigned long long>::max());
	addParameter("Advection scaling", Continuous, 1.0);
	addParameter("Propagation scaling", Continuous, 1.0);
	addParameter("Curvature scaling", Continuous, 1.0);
	addParameter("Iso surface value", Continuous, 0.5);
	addParameter("Reverse expansion direction", Boolean, false);
}

void iALaplacianSegmentationLevelSet::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(laplacianSegmentationLevelSet, inputPixelType(), this, parameters);
}



template<class T>
void cannySegmentationLevelSet(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using FeatureImageType = itk::Image<T, DIM>;
	using OutputPixelType = float;
	using LevelSetFilter = itk::CannySegmentationLevelSetImageFilter<InputImageType, FeatureImageType, OutputPixelType>;
	auto cannySegmentation = LevelSetFilter::New();
	cannySegmentation->SetInput(dynamic_cast<InputImageType*>(filter->input()[0]->itkImage()));
	cannySegmentation->SetFeatureImage(dynamic_cast<FeatureImageType*>(filter->input()[1]->itkImage()));
	cannySegmentation->SetMaximumRMSError(parameters["Maximum RMS error"].toDouble());
	cannySegmentation->SetNumberOfIterations(parameters["Number of iterations"].toULongLong());
	cannySegmentation->SetAdvectionScaling(parameters["Advection scaling"].toDouble());
	cannySegmentation->SetPropagationScaling(parameters["Propagation scaling"].toDouble());
	cannySegmentation->SetCurvatureScaling(parameters["Curvature scaling"].toDouble());
	cannySegmentation->SetIsoSurfaceValue(parameters["Iso surface value"].toDouble());
	cannySegmentation->SetReverseExpansionDirection(parameters["Reverse expansion direction"].toBool());
	filter->progress()->observe(cannySegmentation);
	cannySegmentation->Update();
	filter->addOutput(cannySegmentation->GetOutput());
}

IAFILTER_CREATE(iACannySegmentationLevelSet)

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
	addParameter("Maximum RMS error", Continuous, 0.002, 0.0, 1.0);
	addParameter("Number of iterations", Discrete, 1000, 1, std::numeric_limits<unsigned long long>::max());
	addParameter("Advection scaling", Continuous, 1.0);
	addParameter("Propagation scaling", Continuous, 1.0);
	addParameter("Curvature scaling", Continuous, 1.0);
	addParameter("Iso surface value", Continuous, 0.5);
	addParameter("Reverse expansion direction", Boolean, false);
}

void iACannySegmentationLevelSet::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(cannySegmentationLevelSet, inputPixelType(), this, parameters);
}



template<class T>
void zeroCrossing(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using ZeroCrossingFilter = itk::ZeroCrossingImageFilter<InputImageType, InputImageType>;
	auto zeroCrossingFilter = ZeroCrossingFilter::New();
	zeroCrossingFilter->SetInput(dynamic_cast<InputImageType*>(filter->input()[0]->itkImage()));
	zeroCrossingFilter->SetForegroundValue(parameters["Foreground value"].toDouble());
	zeroCrossingFilter->SetBackgroundValue(parameters["Background value"].toDouble());
	filter->progress()->observe(zeroCrossingFilter);
	zeroCrossingFilter->Update();
	filter->addOutput(zeroCrossingFilter->GetOutput());
}

IAFILTER_CREATE(iAZeroCrossing)

iAZeroCrossing::iAZeroCrossing() :
	iAFilter("Zero Crossing", "Segmentation/Level Set",
		"Finds the closest pixel to the zero-crossings (i.e., sign changes) in a given image.<br/>"
		"Pixels closest to zero-crossings are labeled with a foreground value, all other with a background value."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ZeroCrossingImageFilter.html#details\">"
		"Zero Crossing Image Filter</a> in the ITK documentation.")
{
	addParameter("Background value", Continuous, 0);
	addParameter("Foreground value", Continuous, 1);
}

void iAZeroCrossing::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(zeroCrossing, inputPixelType(), this, parameters);
}

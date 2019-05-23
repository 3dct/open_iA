/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAIntensity.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkAdaptiveHistogramEqualizationImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkHistogramMatchingImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkShiftScaleImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkTestingComparisonImageFilter.h>
#include <itkThresholdImageFilter.h>

// Filters requiring 1 input image:


// iAInvertIntensityFilter

template<class T> void invert_intensity(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::InvertIntensityImageFilter< ImageType, ImageType> InvertFilterType;

	auto invFilter = InvertFilterType::New();
	invFilter->SetInput(0, dynamic_cast< ImageType * >(filter->Input()[0]->GetITKImage()));
	if (parameters["Set Maximum"].toBool())
	{
		invFilter->SetMaximum(parameters["Maximum"].toDouble());
	}
	filter->Progress()->Observe(invFilter);
	invFilter->Update();
	filter->AddOutput(invFilter->GetOutput());
}

void iAInvertIntensityFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(invert_intensity, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAInvertIntensityFilter)

iAInvertIntensityFilter::iAInvertIntensityFilter() :
	iAFilter("Invert", "Intensity",
		"Inverts all intensity values in the image, by subtracting each voxel value from a maximum.<br/>"
		"If you <em>Set Maximum</em>, then the <em>Maximum</em> value will be used, otherwise it "
		"defaults to the maximum of the input pixel type.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1InvertIntensityImageFilter.html\">"
		"Invert Intensity Filter</a> in the ITK documentation.")
{
	AddParameter("Set Maximum", Boolean, false);
	AddParameter("Maximum", Continuous, 65535);
}


// iANormalizeIntensityFilter

template<class T> void normalize(iAFilter* filter)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;

	auto normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput(dynamic_cast< ImageType * >(filter->Input()[0]->GetITKImage()));
	normalizeFilter->Update();
	filter->Progress()->Observe(normalizeFilter);
	normalizeFilter->Update();
	filter->AddOutput(normalizeFilter->GetOutput());
}

void iANormalizeIntensityFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(normalize, InputPixelType(), this);
}

IAFILTER_CREATE(iANormalizeIntensityFilter)

iANormalizeIntensityFilter::iANormalizeIntensityFilter() :
	iAFilter("Normalize Image", "Intensity",
		"Normalize an image by setting its mean to zero and variance to one.<br/>"
		"The Normalize Image Filter shifts and scales an image so that the pixels in the image "
		"have a zero mean and unit variance. <strong>Note:</strong> since this filter normalizes the data to lie within -1 to "
		"1, integral types will produce an image that DOES NOT HAVE a unit variance.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizeImageFilter.html\">"
		"Normalize Image Filter</a> in the ITK documentation.")
{	// no parameters
}


// iAIntensityWindowingFilter

template<class T>
void intensity_windowing(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::IntensityWindowingImageFilter <ImageType, ImageType> IntensityWindowingImageFilterType;

	auto intensityWindowingFilter = IntensityWindowingImageFilterType::New();
	intensityWindowingFilter->SetInput(dynamic_cast< ImageType * >(filter->Input()[0]->GetITKImage()));
	intensityWindowingFilter->SetWindowMinimum(parameters["Window Minimum"].toDouble());
	intensityWindowingFilter->SetWindowMaximum(parameters["Window Maximum"].toDouble());
	intensityWindowingFilter->SetOutputMinimum(parameters["Output Minimum"].toDouble());
	intensityWindowingFilter->SetOutputMaximum(parameters["Output Maximum"].toDouble());
	intensityWindowingFilter->Update();
	filter->Progress()->Observe(intensityWindowingFilter);
	intensityWindowingFilter->Update();
	filter->AddOutput(intensityWindowingFilter->GetOutput());
}

void iAIntensityWindowingFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(intensity_windowing, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAIntensityWindowingFilter)

iAIntensityWindowingFilter::iAIntensityWindowingFilter() :
	iAFilter("Intensity Windowing", "Intensity",
		"Applies a linear transformation to the intensity levels of an image.<br/>"
		"This filter applies pixel-wise a linear transformation to the intensity "
		"values of input image pixels. The linear transformation is defined by "
		"the user in terms of the minimum and maximum values that the output image "
		"should have and the lower and upper limits of the intensity window of the "
		"input image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1IntensityWindowingImageFilter.html\">"
		"Intensity Windowing Filter</a> in the ITK documentation.")
{
	AddParameter("Window Minimum", Continuous, 0);
	AddParameter("Window Maximum", Continuous, 1);
	AddParameter("Output Minimum", Continuous, 0);
	AddParameter("Output Maximum", Continuous, 1);
}


// iAGeneralThreshold

template<class T> void threshold(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, 3 >   ImageType;
	typedef itk::ThresholdImageFilter <ImageType> ThresholdFilterType;
	auto thresholdFilter = ThresholdFilterType::New();
	thresholdFilter->SetOutsideValue( parameters["Outside value"].toDouble() );
	thresholdFilter->ThresholdOutside( parameters["Lower threshold"].toDouble(),
			parameters["Upper threshold"].toDouble());
	thresholdFilter->SetInput( dynamic_cast< ImageType * >( filter->Input()[0]->GetITKImage() ) );
	filter->Progress()->Observe( thresholdFilter );
	thresholdFilter->Update();
	filter->AddOutput(thresholdFilter->GetOutput());
}

void iAGeneralThreshold::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(threshold, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAGeneralThreshold)

iAGeneralThreshold::iAGeneralThreshold() :
	iAFilter("General threshold filter", "Intensity",
		"Set image values to <em>Outside value</em> if they are outside of the given interval.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ThresholdImageFilter.html\">"
		"Threshold Filter</a> in the ITK documentation.")
{
	AddParameter("Lower threshold", Continuous, 0);
	AddParameter("Upper threshold", Continuous, 1);
	AddParameter("Outside value", Continuous, 0);
}



// iARescaleIntensityFilter

template<class T> void rescaleImage(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::RescaleIntensityImageFilter< InputImageType, OutputImageType > RescalerType;

	auto rescaleFilter = RescalerType::New();
	rescaleFilter->SetInput(dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()));
	rescaleFilter->SetOutputMinimum(parameters["Output Minimum"].toDouble());
	rescaleFilter->SetOutputMaximum(parameters["Output Maximum"].toDouble());
	filter->Progress()->Observe(rescaleFilter);
	rescaleFilter->Update();
	filter->AddOutput(rescaleFilter->GetOutput());
}

void iARescaleIntensityFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(rescaleImage, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iARescaleIntensityFilter)

iARescaleIntensityFilter::iARescaleIntensityFilter() :
	iAFilter("Rescale Intensities", "Intensity",
		"Applies a linear transformation to the intensity levels of an image.<br/>"
		"This filter applies pixel-wise a linear transformation to the intensity values "
		"of input image pixels. The linear transformation is defined by the user in "
		"terms of the minimum and maximum values that the output image should have.<br/>"
		"<strong>Note:</strong> In this filter, the minimum and maximum values of "
		"the input image are computed internally (as the minimum and maximum intensity "
		"values occurring in the image). If you need a filter where you can "
		"set the minimum and maximum values of the input, please use the <em>Intensity "
		"windowing</em> or the <em>Datatype Conversion</em> filter. If you want a filter that "
		"can perform a user-defined linear transformation on the intensity, then please use "
		"the <em>Shift scale</em> filter.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RescaleIntensityImageFilter.html\">"
		"Rescale Intensity Filter</a> in the ITK documentation.")
{
	AddParameter("Output Minimum", Continuous, 0);
	AddParameter("Output Maximum", Continuous, 1);
}


// iAShiftScaleIntensityFilter

template<class T> void shiftScale(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::ShiftScaleImageFilter< InputImageType, OutputImageType > RescalerType;

	auto rescaleFilter = RescalerType::New();
	rescaleFilter->SetInput(dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()));
	rescaleFilter->SetShift(parameters["Shift"].toDouble());
	rescaleFilter->SetScale(parameters["Scale"].toDouble());
	filter->Progress()->Observe(rescaleFilter);
	rescaleFilter->Update();
	filter->AddOutput(rescaleFilter->GetOutput());
}

void iAShiftScaleIntensityFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(shiftScale, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAShiftScaleIntensityFilter)

iAShiftScaleIntensityFilter::iAShiftScaleIntensityFilter() :
	iAFilter("Shift and Scale", "Intensity",
		"Shift and scale the pixels in an image.<br/>"
		"Shifts the input pixel by <em>Shift</em> and then scales the pixel by "
		"<em>Scale</em>. All computations are performed in the precision of "
		"the input pixel type.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ShiftScaleImageFilter.html\">"
		"Shift Scale Filter</a> in the ITK documentation.")
{
	AddParameter("Shift", Continuous, 0);
	AddParameter("Scale", Continuous, 1);
}


template<class T> void adaptiveHistogramEqualization(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef  itk::AdaptiveHistogramEqualizationImageFilter< InputImageType > AdaptHistoEqualFilterType;
	auto adaptHistoEqualFilter = AdaptHistoEqualFilterType::New();
	adaptHistoEqualFilter->SetInput(dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()));
	adaptHistoEqualFilter->SetAlpha(params["Alpha"].toDouble());
	adaptHistoEqualFilter->SetBeta(params["Beta"].toDouble());
	adaptHistoEqualFilter->SetRadius(params["Radius"].toUInt());
	filter->Progress()->Observe(adaptHistoEqualFilter);
	adaptHistoEqualFilter->Update();
	filter->AddOutput(adaptHistoEqualFilter->GetOutput());
}

IAFILTER_CREATE(iAAdaptiveHistogramEqualization)

void iAAdaptiveHistogramEqualization::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(adaptiveHistogramEqualization, InputPixelType(), this, parameters);
}

iAAdaptiveHistogramEqualization::iAAdaptiveHistogramEqualization() :
	iAFilter("Adaptive Histogram Equalization", "Intensity",
		"This filter is a superset of many contrast enhancing filters.<br/>"
		"By modifying its parameters (Alpha, Beta), the filter can produce an "
		"adaptively equalized histogram or a version of unsharp mask (local "
		"mean subtraction).<br/>"
		"<em>Alpha</em> controls how much the filter acts like the "
		"classical histogram equalization method (Alpha=0) to how much the "
		"filter acts like an unsharp mask (Alpha=1). The parameter <em>Beta</em> "
		"controls how much the filter acts like an unsharp mask (Beta=0) to "
		"how much the filter acts like pass through (Beta=1, with Alpha=1)."
		"<em>Radius</em> specifies the size of the region (in voxels) around "
		"the current voxel used for filtering."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AdaptiveHistogramEqualizationImageFilter.html\">"
		"Adaptive Histogram Equalization Filter</a> in the ITK documentation.")
{
	AddParameter("Alpha", Continuous, 0, 0, 1);
	AddParameter("Beta", Continuous, 0, 0, 1);
	AddParameter("Radius", Discrete, 5, 1);
}




// Filters requiring 2 input images:


// iAAddFilter

template<class T> void addImages(iAFilter* filter)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::AddImageFilter<InputImageType, InputImageType> AddImageFilter;

	auto fusion = AddImageFilter::New();
	fusion->InPlaceOff();
	fusion->SetInput1(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()));
	auto img2 = CastImageTo<T>(filter->Input()[1]->GetITKImage());
	fusion->SetInput2(dynamic_cast<InputImageType *>(img2.GetPointer()));
	filter->Progress()->Observe(fusion);
	fusion->Update();
	filter->AddOutput(fusion->GetOutput());
}

void iAAddFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(addImages, InputPixelType(), this);
}

IAFILTER_CREATE(iAAddFilter)

iAAddFilter::iAAddFilter() :
	iAFilter("Add Images", "Intensity",
		"Adds the intensities at each element of the two given images.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AddImageFilter.html\">"
		"Add Image Filter</a> in the ITK documentation.", 2)
{	// no parameters
}


// iASubtractFilter

template<class T> void subtractImages(iAFilter* filter)
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< T, 3 > OutputImageType;
	typedef itk::SubtractImageFilter<InputImageType, InputImageType, OutputImageType> SubractFilterType;

	auto subFilter = SubractFilterType::New();
	subFilter->SetInput1(dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()));
	auto img2 = CastImageTo<T>(filter->Input()[1]->GetITKImage());
	subFilter->SetInput2(dynamic_cast<InputImageType *>(img2.GetPointer()));
	filter->Progress()->Observe(subFilter);
	subFilter->Update();
	filter->AddOutput(subFilter->GetOutput());
}

void iASubtractFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(subtractImages, InputPixelType(), this);
}

IAFILTER_CREATE(iASubtractFilter)

iASubtractFilter::iASubtractFilter() :
	iAFilter("Subtract Images", "Intensity",
		"Subtracts the intensities of the selected <em>Additional Image</em> from the intensities of the active window.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SubtractImageFilter.html\">"
		"Subtract Image Filter</a> in the ITK documentation.", 2)
{	// no parameters
}


// iADifferenceFilter

template<class T> void difference(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::Testing::ComparisonImageFilter<ImageType, ImageType> FilterType;

	auto diffFilter = FilterType::New();
	diffFilter->SetDifferenceThreshold(parameters["Difference threshold"].toDouble());
	diffFilter->SetToleranceRadius(parameters["Tolerance radius"].toDouble());
	diffFilter->SetInput(dynamic_cast< ImageType * >(filter->Input()[0]->GetITKImage()));
	auto img2 = CastImageTo<T>(filter->Input()[1]->GetITKImage());
	diffFilter->SetInput(1, dynamic_cast<ImageType *>(img2.GetPointer()));
	filter->Progress()->Observe(diffFilter);
	diffFilter->Update();
	filter->AddOutput(diffFilter->GetOutput());
}

void iADifferenceFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(difference, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iADifferenceFilter)

iADifferenceFilter::iADifferenceFilter() :
	iAFilter("Difference", "Intensity",
		"Computes the difference between the intensities in the active window and the selected <em>Additional Image</em>.<br/>"
		"What is considered a difference can be influenced by the <em>Difference threshold</em> "
		"(the minimum threshold for pixels to be different) "
		"and the <em>Tolerance radius</em> (the maximum distance to look for matching pixel) Parameters.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1Testing_1_1ComparisonImageFilter.html\">"
		"Testing Comparison Filter</a> in the ITK documentation.", 2)
{
	AddParameter("Difference threshold", Continuous, 0);
	AddParameter("Tolerance radius", Continuous, 0);
}


// iAMaskIntensityFilter

template<class T> void mask(iAFilter* filter)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::MaskImageFilter< ImageType, ImageType > MaskFilterType;

	auto maskFilter = MaskFilterType::New();
	maskFilter->SetInput(dynamic_cast< ImageType * >(filter->Input()[0]->GetITKImage()));
	maskFilter->SetMaskImage(dynamic_cast< ImageType * >(filter->Input()[1]->GetITKImage()));
	filter->Progress()->Observe(maskFilter);
	maskFilter->Update();
	filter->AddOutput(maskFilter->GetOutput());
}

void iAMaskIntensityFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(mask, InputPixelType(), this);
}

IAFILTER_CREATE(iAMaskIntensityFilter)

iAMaskIntensityFilter::iAMaskIntensityFilter() :
	iAFilter("Mask Image", "Intensity",
		"Mask an image with a mask image.<br/>"
		"The output image will contain the values of the first input image, "
		"but only in those places where the mask images has a value other "
		"than 0.<br/>"
		"Note: This filter expects both input images to have the same type!<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MaskImageFilter.html\">"
		"Mask Image Filter</a> in the ITK documentation.", 2)
{	// no parameters
}


// iAHistogramMatchingFilter

template<class T>
void histomatch(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef double InternalPixelType;
	typedef itk::Image< InternalPixelType, DIM > InternalImageType;
	typedef itk::CastImageFilter< ImageType, InternalImageType > CasterType;
	typedef itk::HistogramMatchingImageFilter<InternalImageType, InternalImageType > MatchingFilterType;

	auto fixedImageCaster = CasterType::New();
	auto movingImageCaster = CasterType::New();
	fixedImageCaster->SetInput( dynamic_cast< ImageType * >( filter->Input()[0]->GetITKImage() ) );
	movingImageCaster->SetInput( dynamic_cast< ImageType * >( filter->Input()[1]->GetITKImage() ) );
	auto matcher = MatchingFilterType::New();
	matcher->SetInput( movingImageCaster->GetOutput() );
	matcher->SetReferenceImage( fixedImageCaster->GetOutput() );
	matcher->SetNumberOfHistogramLevels(parameters["Number of histogram levels"].toUInt() );
	matcher->SetNumberOfMatchPoints(parameters["Number of match points"].toUInt());
	if (parameters["Threshold at mean intensity"].toBool())
	{
		matcher->ThresholdAtMeanIntensityOn();
	}
	filter->Progress()->Observe( matcher );
	matcher->Update();
	filter->AddOutput( matcher->GetOutput() );
}

void iAHistogramMatchingFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(histomatch, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAHistogramMatchingFilter)

iAHistogramMatchingFilter::iAHistogramMatchingFilter() :
	iAFilter("Histogram Match", "Intensity",
		"Normalizes the grayscale values between two images by histogram matching.<br/>"
		"The Histogram Matching Image Filter normalizes the grayscale values of a source "
		"image based on the grayscale values of a reference image. This filter uses a histogram "
		"matching technique where the histograms of the two images are matched only at a specified "
		"number of quantile values.<br/>"
		"This filter was originally designed to normalize MR images of the same MR protocol and "
		"same body part. The algorithm works best if background pixels are excluded from both the "
		"source and reference histograms. A simple background exclusion method is to exclude all "
		"pixels whose grayscale values are smaller than the mean grayscale value. "
		"<em>Threshold at mean intensity</em> switches on this simple background exclusion method.</br>"
		"The <em>Number of Histogram Levels</em> sets the number of bins used when creating histograms of the "
		"source and reference images. The <em>Number of match points</em> governs the number of quantile values "
		"to be matched.<br/>"
		"This filter assumes that both the source and reference are of the same type and that the "
		"input and output image type have the same number of dimension and have scalar pixel types.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HistogramMatchingImageFilter.html\">"
		"Histogram Matching Filter</a> in the ITK documentation.", 2)
{
	AddParameter("Threshold at mean intensity", Boolean, true);
	AddParameter("Number of histogram levels", Continuous, 256);
	AddParameter("Number of match points", Continuous, 1);
}

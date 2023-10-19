// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>          // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkAdaptiveHistogramEqualizationImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkHistogramMatchingImageFilter.h>
#include <itkImageIterator.h>
#include <itkImageRegionIterator.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkMultiplyImageFilter.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkShiftScaleImageFilter.h>
#include <itkSubtractImageFilter.h>
#include <itkTestingComparisonImageFilter.h>
#include <itkThresholdImageFilter.h>

// Filters requiring 1 input image:
IAFILTER_DEFAULT_CLASS(iAAdaptiveHistogramEqualization);
IAFILTER_DEFAULT_CLASS(iAGeneralThreshold);
IAFILTER_DEFAULT_CLASS(iAIntensityWindowingFilter);
IAFILTER_DEFAULT_CLASS(iAInvertIntensityFilter);
IAFILTER_DEFAULT_CLASS(iAMaskIntensityFilter);
IAFILTER_DEFAULT_CLASS(iANormalizeIntensityFilter);
IAFILTER_DEFAULT_CLASS(iARescaleIntensityFilter);
IAFILTER_DEFAULT_CLASS(iAShiftScaleIntensityFilter);
IAFILTER_DEFAULT_CLASS(iAHistogramFill);
IAFILTER_DEFAULT_CLASS(iAReplaceAndShiftFilter);
// Filters requiring 2 input images:
IAFILTER_DEFAULT_CLASS(iAAddFilter);
IAFILTER_DEFAULT_CLASS(iADifferenceFilter);
IAFILTER_DEFAULT_CLASS(iAMultiplyFilter);
IAFILTER_DEFAULT_CLASS(iASubtractFilter);
IAFILTER_DEFAULT_CLASS(iAHistogramMatchingFilter);


// iAInvertIntensityFilter

template<class T> void invert_intensity(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::InvertIntensityImageFilter< ImageType, ImageType> InvertFilterType;

	auto invFilter = InvertFilterType::New();
	invFilter->SetInput(0, dynamic_cast< ImageType * >(filter->imageInput(0)->itkImage()));
	if (parameters["Set Maximum"].toBool())
	{
		invFilter->SetMaximum(parameters["Maximum"].toDouble());
	}
	filter->progress()->observe(invFilter);
	invFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(invFilter->GetOutput()));
}

void iAInvertIntensityFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(invert_intensity, inputScalarType(), this, parameters);
}

iAInvertIntensityFilter::iAInvertIntensityFilter() :
	iAFilter("Invert", "Intensity",
		"Inverts all intensity values in the image, by subtracting each voxel value from a maximum.<br/>"
		"If you <em>Set Maximum</em>, then the <em>Maximum</em> value will be used, otherwise it "
		"defaults to the maximum of the input pixel type.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1InvertIntensityImageFilter.html\">"
		"Invert Intensity Filter</a> in the ITK documentation.")
{
	addParameter("Set Maximum", iAValueType::Boolean, false);
	addParameter("Maximum", iAValueType::Continuous, 65535);
}


// iANormalizeIntensityFilter

template<class T> void normalize(iAFilter* filter)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;

	auto normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput(dynamic_cast< ImageType * >(filter->imageInput(0)->itkImage()));
	normalizeFilter->Update();
	filter->progress()->observe(normalizeFilter);
	normalizeFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(normalizeFilter->GetOutput()));
}

void iANormalizeIntensityFilter::performWork(QVariantMap const & /*parameters*/)
{
	ITK_TYPED_CALL(normalize, inputScalarType(), this);
}

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
void intensity_windowing(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::IntensityWindowingImageFilter <ImageType, ImageType> IntensityWindowingImageFilterType;

	auto intensityWindowingFilter = IntensityWindowingImageFilterType::New();
	intensityWindowingFilter->SetInput(dynamic_cast< ImageType * >(filter->imageInput(0)->itkImage()));
	intensityWindowingFilter->SetWindowMinimum(parameters["Window Minimum"].toDouble());
	intensityWindowingFilter->SetWindowMaximum(parameters["Window Maximum"].toDouble());
	intensityWindowingFilter->SetOutputMinimum(parameters["Output Minimum"].toDouble());
	intensityWindowingFilter->SetOutputMaximum(parameters["Output Maximum"].toDouble());
	intensityWindowingFilter->Update();
	filter->progress()->observe(intensityWindowingFilter);
	intensityWindowingFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(intensityWindowingFilter->GetOutput()));
}

void iAIntensityWindowingFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(intensity_windowing, inputScalarType(), this, parameters);
}

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
	addParameter("Window Minimum", iAValueType::Continuous, 0);
	addParameter("Window Maximum", iAValueType::Continuous, 1);
	addParameter("Output Minimum", iAValueType::Continuous, 0);
	addParameter("Output Maximum", iAValueType::Continuous, 1);
}


// iAGeneralThreshold

template<class T> void threshold(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, 3 >   ImageType;
	typedef itk::ThresholdImageFilter <ImageType> ThresholdFilterType;
	auto thresholdFilter = ThresholdFilterType::New();
	thresholdFilter->SetOutsideValue( parameters["Outside value"].toDouble() );
	thresholdFilter->ThresholdOutside( parameters["Lower threshold"].toDouble(),
			parameters["Upper threshold"].toDouble());
	thresholdFilter->SetInput( dynamic_cast< ImageType * >( filter->imageInput(0)->itkImage() ) );
	filter->progress()->observe( thresholdFilter );
	thresholdFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(thresholdFilter->GetOutput()));
}

void iAGeneralThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(threshold, inputScalarType(), this, parameters);
}

iAGeneralThreshold::iAGeneralThreshold() :
	iAFilter("General threshold filter", "Intensity",
		"Set image values to <em>Outside value</em> if they are outside of the given interval.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ThresholdImageFilter.html\">"
		"Threshold Filter</a> in the ITK documentation.")
{
	addParameter("Lower threshold", iAValueType::Continuous, 0);
	addParameter("Upper threshold", iAValueType::Continuous, 1);
	addParameter("Outside value", iAValueType::Continuous, 0);
}



// iARescaleIntensityFilter

template<class T> void rescaleImage(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::RescaleIntensityImageFilter< InputImageType, OutputImageType > RescalerType;

	auto rescaleFilter = RescalerType::New();
	rescaleFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	rescaleFilter->SetOutputMinimum(parameters["Output Minimum"].toDouble());
	rescaleFilter->SetOutputMaximum(parameters["Output Maximum"].toDouble());
	filter->progress()->observe(rescaleFilter);
	rescaleFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(rescaleFilter->GetOutput()));
}

void iARescaleIntensityFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(rescaleImage, inputScalarType(), this, parameters);
}

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
	addParameter("Output Minimum", iAValueType::Continuous, 0);
	addParameter("Output Maximum", iAValueType::Continuous, 1);
}


// iAShiftScaleIntensityFilter

template<typename T> void shiftScale(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::ShiftScaleImageFilter< InputImageType, OutputImageType > RescalerType;

	auto rescaleFilter = RescalerType::New();
	rescaleFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	rescaleFilter->SetShift(parameters["Shift"].toDouble());
	rescaleFilter->SetScale(parameters["Scale"].toDouble());
	filter->progress()->observe(rescaleFilter);
	rescaleFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(rescaleFilter->GetOutput()));
}

void iAShiftScaleIntensityFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(shiftScale, inputScalarType(), this, parameters);
}

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
	addParameter("Shift", iAValueType::Continuous, 0);
	addParameter("Scale", iAValueType::Continuous, 1);
}


template<class T> void adaptiveHistogramEqualization(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef  itk::AdaptiveHistogramEqualizationImageFilter< InputImageType > AdaptHistoEqualFilterType;
	auto adaptHistoEqualFilter = AdaptHistoEqualFilterType::New();
	adaptHistoEqualFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	adaptHistoEqualFilter->SetAlpha(params["Alpha"].toDouble());
	adaptHistoEqualFilter->SetBeta(params["Beta"].toDouble());
	adaptHistoEqualFilter->SetRadius(params["Radius"].toUInt());
	filter->progress()->observe(adaptHistoEqualFilter);
	adaptHistoEqualFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(adaptHistoEqualFilter->GetOutput()));
}

void iAAdaptiveHistogramEqualization::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(adaptiveHistogramEqualization, inputScalarType(), this, parameters);
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
	addParameter("Alpha", iAValueType::Continuous, 0, 0, 1);
	addParameter("Beta", iAValueType::Continuous, 0, 0, 1);
	addParameter("Radius", iAValueType::Discrete, 5, 1);
}


// iAReplaceValueFilter

template<class T> 
void replaceAndShift(iAFilter* filter, QVariantMap const & params)
{
	using ImageType = itk::Image<T, DIM>;
	typedef itk::ImageRegionIterator<ImageType> ImageIterator;
	auto im = dynamic_cast<ImageType*>(filter->imageInput(0)->itkImage());
	typename ImageType::RegionType region = im->GetLargestPossibleRegion();
	auto imgOut = ImageType::New();
	imgOut->SetRegions(region);
	imgOut->Allocate();

	ImageIterator it(im, im->GetRequestedRegion());
	ImageIterator itOut(imgOut, imgOut->GetRequestedRegion());

	auto valueToReplace = params["Value To Replace"].value<T>();
	auto replacement = params["Replacement"].value<T>();
	if (valueToReplace == replacement)
	{
		LOG(lvlWarn, "Parameters 'Value To Replace' and 'Replacement' may not have the same value!");
		return;
	}
	auto start = valueToReplace < replacement ? valueToReplace + 1 : replacement;
	auto end   = valueToReplace > replacement ? valueToReplace - 1 : replacement;
	T ofs = replacement < valueToReplace ? +1 : -1;

	for (it.GoToBegin(); !it.IsAtEnd(); ++it, ++itOut)
	{
		if (it.Value() == valueToReplace)
		{
			itOut.Set(replacement);
		}
		else if (it.Value() >= start && it.Value() <= end)
		{
			itOut.Set(it.Value() + ofs);
		}
		else
		{
			itOut.Set(it.Value());
		}
	}
	filter->addOutput(std::make_shared<iAImageData>(imgOut));
}

void iAReplaceAndShiftFilter::performWork(QVariantMap const& parameters)
{
	if (inputScalarType() == iAITKIO::ScalarType::FLOAT ||
		inputScalarType() == iAITKIO::ScalarType::DOUBLE)
	{
		LOG(lvlWarn, "Replace and shift is executed on a real-valued (float/double) input image. "
			"This only makes sense if the input image only contains discrete values; "
			"and even then, comparisons might fail. You have been warned!");
	}
	ITK_TYPED_CALL(replaceAndShift, inputScalarType(), this, parameters);
}

iAReplaceAndShiftFilter::iAReplaceAndShiftFilter() :
	iAFilter("Replace and Shift", "Intensity",
		"Replace one intensity value by another, and shift values in between accordingly.<br/>"
		"The intensity value specified by <em>Value To Replace</em> is replaced by the value specified as <em>Replacement</em>."
		"All values between the two (including the <em>Replacement</em> but excluding the <em>Value to Replace</em> are shifted by one in direction of the <em>Value to Replace</em>.<br/>"
		"Only makes sense for discrete value types (not for floating point images).")
{
	addParameter("Value To Replace", iAValueType::Discrete, 0, 0);
	addParameter("Replacement", iAValueType::Discrete, 0, 0);
}




// iAAddFilter

template<class T> void addImages(iAFilter* filter)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::AddImageFilter<InputImageType, InputImageType> AddImageFilter;

	auto fusion = AddImageFilter::New();
	fusion->InPlaceOff();
	fusion->SetInput1(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	auto img2 = castImageTo<T>(filter->imageInput(1)->itkImage());
	fusion->SetInput2(dynamic_cast<InputImageType *>(img2.GetPointer()));
	filter->progress()->observe(fusion);
	fusion->Update();
	filter->addOutput(std::make_shared<iAImageData>(fusion->GetOutput()));
}

void iAAddFilter::performWork(QVariantMap const & /*parameters*/)
{
	ITK_TYPED_CALL(addImages, inputScalarType(), this);
}

iAAddFilter::iAAddFilter() :
	iAFilter("Add Images", "Intensity",
		"Adds the intensities at each element of the two given images.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AddImageFilter.html\">"
		"Add Image Filter</a> in the ITK documentation.", 2)
{	// no parameters
	setInputName(1u, "Second image");
}

// iAMultiplyFilter

template <class T>
void multiplyImages(iAFilter* filter)
{
	typedef itk::Image<T, 3> InputImageType;
	typedef itk::Image<T, 3> OutputImageType;
	typedef itk::MultiplyImageFilter<InputImageType, InputImageType, OutputImageType> MultiplyFilterType;

	auto mulFilter = MultiplyFilterType::New();
	mulFilter->SetInput1(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	auto img2 = castImageTo<T>(filter->imageInput(1)->itkImage());
	mulFilter->SetInput2(dynamic_cast<InputImageType*>(img2.GetPointer()));
	filter->progress()->observe(mulFilter);
	mulFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(mulFilter->GetOutput()));
}

void iAMultiplyFilter::performWork(QVariantMap const& /*parameters*/)
{
	ITK_TYPED_CALL(multiplyImages, inputScalarType(), this);
}

iAMultiplyFilter::iAMultiplyFilter() :
	iAFilter("Multiply Images", "Intensity",
		"Multiplies the intensities of the selected <em>Additional Image</em> with the intensities of the active "
		"window.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MultiplyImageFilter.html\">"
		"Multiply Image Filter</a> in the ITK documentation.",
		2)
{  // no parameters
	setInputName(1u, "Additional image");
}


// iASubtractFilter

template<class T> void subtractImages(iAFilter* filter)
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< T, 3 > OutputImageType;
	typedef itk::SubtractImageFilter<InputImageType, InputImageType, OutputImageType> SubtractFilterType;

	auto subFilter = SubtractFilterType::New();
	subFilter->SetInput1(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	auto img2 = castImageTo<T>(filter->imageInput(1)->itkImage());
	subFilter->SetInput2(dynamic_cast<InputImageType *>(img2.GetPointer()));
	filter->progress()->observe(subFilter);
	subFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(subFilter->GetOutput()));
}

void iASubtractFilter::performWork(QVariantMap const & /*parameters*/)
{
	ITK_TYPED_CALL(subtractImages, inputScalarType(), this);
}

iASubtractFilter::iASubtractFilter() :
	iAFilter("Subtract Images", "Intensity",
		"Subtracts the intensities of the selected <em>Additional Image</em> from the intensities of the active window.<br/>"
		"Note: The second image will be converted to the pixel type of the first.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SubtractImageFilter.html\">"
		"Subtract Image Filter</a> in the ITK documentation.", 2)
{	// no parameters
	setInputName(1u, "Additional image");
}


// iADifferenceFilter

template<class T> void difference(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::Testing::ComparisonImageFilter<ImageType, ImageType> FilterType;

	auto diffFilter = FilterType::New();
	diffFilter->SetDifferenceThreshold(parameters["Difference threshold"].toDouble());
	diffFilter->SetToleranceRadius(parameters["Tolerance radius"].toDouble());
	diffFilter->SetInput(dynamic_cast< ImageType * >(filter->imageInput(0)->itkImage()));
	auto img2 = castImageTo<T>(filter->imageInput(1)->itkImage());
	diffFilter->SetInput(1, dynamic_cast<ImageType *>(img2.GetPointer()));
	filter->progress()->observe(diffFilter);
	diffFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(diffFilter->GetOutput()));
}

void iADifferenceFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(difference, inputScalarType(), this, parameters);
}

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
	addParameter("Difference threshold", iAValueType::Continuous, 0);
	addParameter("Tolerance radius", iAValueType::Continuous, 0);
	setInputName(1u, "Additional image");
}


// iAMaskIntensityFilter

template<class T> void mask(iAFilter* filter)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::MaskImageFilter< ImageType, ImageType > MaskFilterType;

	auto maskFilter = MaskFilterType::New();
	maskFilter->SetInput(dynamic_cast< ImageType * >(filter->imageInput(0)->itkImage()));
	maskFilter->SetMaskImage(dynamic_cast< ImageType * >(filter->imageInput(1)->itkImage()));
	filter->progress()->observe(maskFilter);
	maskFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(maskFilter->GetOutput()));
}

void iAMaskIntensityFilter::performWork(QVariantMap const & /*parameters*/)
{
	ITK_TYPED_CALL(mask, inputScalarType(), this);
}

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
	setInputName(1u, "Mask image");
}


// iAHistogramMatchingFilter

template<class T>
void histomatch(iAFilter* filter, QVariantMap const & parameters)
{
	using MatchImageType = itk::Image<T, DIM>;
	using HistoMatchFilterType = itk::HistogramMatchingImageFilter<MatchImageType, MatchImageType>;

	auto matcher = HistoMatchFilterType::New();
	if (itkScalarType(filter->imageInput(0)->itkImage()) != itkScalarType(filter->imageInput(1)->itkImage()))
	{
		LOG(lvlWarn, "Second image does not have the same pixel type as the first; I will try to typecast, "
			"but the filter might not work properly if the data ranges are different.");
	}
	auto refImg = castImageTo<T>(filter->imageInput(1)->itkImage());

	matcher->SetInput(dynamic_cast<MatchImageType*>(filter->imageInput(0)->itkImage()));
	matcher->SetReferenceImage(dynamic_cast<MatchImageType*>(refImg.GetPointer()));
	matcher->SetNumberOfHistogramLevels(parameters["Number of histogram levels"].toUInt() );
	matcher->SetNumberOfMatchPoints(parameters["Number of match points"].toUInt());
	if (parameters["Threshold at mean intensity"].toBool())
	{
		matcher->ThresholdAtMeanIntensityOn();
	}
	filter->progress()->observe( matcher );
	matcher->Update();
	filter->addOutput(std::make_shared<iAImageData>(matcher->GetOutput()) );
}

void iAHistogramMatchingFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(histomatch, inputScalarType(), this, parameters);
}

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
	addParameter("Threshold at mean intensity", iAValueType::Boolean, true);
	addParameter("Number of histogram levels", iAValueType::Continuous, 256);
	addParameter("Number of match points", iAValueType::Continuous, 1);
	setInputName(1u, "Reference image");
}



template <class T>
void fillHistogramm(iAFilter* filter, QVariantMap const& params)
{
	Q_UNUSED(params);
	std::map<T, T> histogramm;
	using ImageType = itk::Image<T, DIM>;
	typename ImageType::Pointer im = dynamic_cast<ImageType*>(filter->imageInput(0)->itkImage());

	using IteratorType = itk::ImageRegionIterator<ImageType>;
	IteratorType it(im, im->GetRequestedRegion());

	it.GoToBegin();
	while (!it.IsAtEnd())
	{
		histogramm[it.Value()] = it.Value();
		++it;
	}

	int index = 0;
	for (auto& element : histogramm)
	{
		histogramm[element.first] = index;
		index++;
	}
	filter->progress()->emitProgress(50);

	it.GoToBegin();
	while (!it.IsAtEnd())
	{
		it.Set(histogramm[it.Value()]);
		++it;
	}
}

iAHistogramFill::iAHistogramFill() :
	iAFilter("Histogram Fill", "Intensity",
		"Packs a histogram so that consecutive values starting from 0 are used.<br/>"
		"Only useful for integer images. For example, if you have an image containing the values "
		"2, 5, 6 and 8, these would be translated to 0, 1, 2 and 3 respectively (2 -> 0, 5 -> 1, 6 -> 2, 8 -> 3).")
{
}

void iAHistogramFill::performWork(QVariantMap const& params)
{
	ITK_TYPED_CALL(fillHistogramm, inputScalarType(), this, params);
}

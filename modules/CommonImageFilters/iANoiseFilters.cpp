// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>    // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkAdditiveGaussianNoiseImageFilter.h>
#include <itkSaltAndPepperNoiseImageFilter.h>
#include <itkShotNoiseImageFilter.h>
#include <itkSpeckleNoiseImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

IAFILTER_DEFAULT_CLASS(iAAdditiveGaussianNoise);
IAFILTER_DEFAULT_CLASS(iASaltAndPepperNoise);
IAFILTER_DEFAULT_CLASS(iAShotNoise);
IAFILTER_DEFAULT_CLASS(iASpeckleNoise);

template<class T> void additiveGaussianNoise(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::AdditiveGaussianNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto noiseFilter = NoiseFilterType::New();
	noiseFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	noiseFilter->SetMean(parameters["Mean"].toDouble());
	noiseFilter->SetStandardDeviation(parameters["Standard deviation"].toDouble());
	filter->progress()->observe( noiseFilter );
	noiseFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(noiseFilter->GetOutput()));
}

void iAAdditiveGaussianNoise::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(additiveGaussianNoise, inputScalarType(), this, parameters);
}

iAAdditiveGaussianNoise::iAAdditiveGaussianNoise() :
	iAFilter("Additive Gaussian", "Noise",
		"Adds additive gaussian white noise to an image.<br/>"
		"To each pixel intensity, a value from a normal distribution with the given "
		"<em>Mean</em> and <em>Standard deviation</em> is added.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AdditiveGaussianNoiseImageFilter.html\">"
		"Additive Gaussian Noise Filter</a> in the ITK documentation.")
{
	addParameter("Mean", iAValueType::Continuous, 0);
	addParameter("Standard deviation", iAValueType::Continuous, 0.1, std::numeric_limits<double>::epsilon());
}



template<class T> void saltAndPepperNoise(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::SaltAndPepperNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto noiseFilter = NoiseFilterType::New();
	noiseFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	noiseFilter->SetProbability(parameters["Probability"].toDouble());
	filter->progress()->observe( noiseFilter );
	noiseFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(noiseFilter->GetOutput()));
}

void iASaltAndPepperNoise::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(saltAndPepperNoise, inputScalarType(), this, parameters);
}

iASaltAndPepperNoise::iASaltAndPepperNoise() :
	iAFilter("Salt and Pepper", "Noise",
		"Alter an image with fixed value impulse noise.<br/>"
		"Salt and pepper noise is a special kind of impulse noise where the value of the noise "
		"is either the maximum possible value in the image or its minimum.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SaltAndPepperNoiseImageFilter.html\">"
		"Salt and Pepper Noise Filter</a> in the ITK documentation.")
{
	addParameter("Probability", iAValueType::Continuous, 0.1, 0, 1);
}



template<class T> void shotNoise(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::ShotNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto noiseFilter = NoiseFilterType::New();
	noiseFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	noiseFilter->SetScale(parameters["Scale"].toDouble());
	filter->progress()->observe( noiseFilter );
	noiseFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(noiseFilter->GetOutput()));
}

void iAShotNoise::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(shotNoise, inputScalarType(), this, parameters);
}

iAShotNoise::iAShotNoise() :
	iAFilter("Shot", "Noise",
		"Alter an image with shot noise.<br/>"
		"The shot noise follows a Poisson distribution with the pixel intensity as mean, "
		"scaled by the given <em>Scale</em> parameter.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ShotNoiseImageFilter.html\">"
		"Shot Noise Filter</a> in the ITK documentation.")
{
	addParameter("Scale", iAValueType::Continuous, 1, std::numeric_limits<double>::epsilon());
}



template<class T> void speckleNoise(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::SpeckleNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto noiseFilter = NoiseFilterType::New();
	noiseFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	noiseFilter->SetStandardDeviation(parameters["Standard deviation"].toDouble());
	filter->progress()->observe( noiseFilter );
	noiseFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(noiseFilter->GetOutput()));
}

void iASpeckleNoise::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(speckleNoise, inputScalarType(), this, parameters);
}

iASpeckleNoise::iASpeckleNoise() :
	iAFilter("Speckle", "Noise",
		"Alter an image with speckle (multiplicative) noise.<br/>"
		"The speckle noise follows a gamma distribution of mean 1 and <em>Standard deviation</em>"
		"provided by the user. The noise is proportional to the pixel intensity.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SpeckleNoiseImageFilter.html\">"
		"Speckle Noise Filter</a> in the ITK documentation.")
{
	addParameter("Standard deviation", iAValueType::Continuous, 0.1, std::numeric_limits<double>::epsilon());
}

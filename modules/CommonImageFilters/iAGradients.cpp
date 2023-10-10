// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h> // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkDerivativeImageFilter.h>
#ifdef ITKHigherOrderGradient
#include <itkHigherOrderAccurateDerivativeImageFilter.h>       // HigherOrderAccurateGradient ITK Module
#endif
#include <itkGradientMagnitudeImageFilter.h>
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>

IAFILTER_DEFAULT_CLASS(iADerivative);
IAFILTER_DEFAULT_CLASS(iAGradientMagnitude);
IAFILTER_DEFAULT_CLASS(iAGradientMagnitudeRecursiveGaussian);
#ifdef ITKHigherOrderGradient
IAFILTER_DEFAULT_CLASS(iAHigherOrderAccurateDerivative);
#endif

// iAGradientMagnitude

template<class T> void gradientMagnitude(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::GradientMagnitudeImageFilter< InputImageType, InputImageType > GMFType;

	auto gmFilter = GMFType::New();
	gmFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	gmFilter->SetUseImageSpacing(params["Use Image Spacing"].toBool());
	filter->progress()->observe(gmFilter);
	gmFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(gmFilter->GetOutput()));
}

void iAGradientMagnitude::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(gradientMagnitude, inputScalarType(), this, parameters);
}

iAGradientMagnitude::iAGradientMagnitude() :
	iAFilter("Gradient Magnitude", "Gradient",
		"Computes the gradient magnitude at each image element.<br/>"
		"If <em>Use Image Spacing</em> is enabled, the gradient is calculated in the physical space; "
		"if it is not enabled, the gradient is calculated in pixel space.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientMagnitudeImageFilter.html\">"
		"Gradient Magnitude Filter</a> in the ITK documentation.")
{
	addParameter("Use Image Spacing", iAValueType::Boolean, true);
}

// iAGradientMagnitudeRecursiveGaussian

template<class T> void gradientMagnitudeRecursiveGaussian(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::GradientMagnitudeRecursiveGaussianImageFilter< InputImageType, InputImageType > GMFType;

	auto gmFilter = GMFType::New();
	gmFilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	gmFilter->SetNormalizeAcrossScale(params["Normalize across scale"].toBool());
	gmFilter->SetSigma(params["Sigma"].toDouble());
	filter->progress()->observe(gmFilter);
	gmFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(gmFilter->GetOutput()));
}

void iAGradientMagnitudeRecursiveGaussian::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(gradientMagnitudeRecursiveGaussian, inputScalarType(), this, parameters);
}

iAGradientMagnitudeRecursiveGaussian::iAGradientMagnitudeRecursiveGaussian() :
	iAFilter("Gradient Magnitude RecursiveGaussian", "Gradient",
		"Computes the gradient magnitude at each image element.<br/>"
		"If <em>Use Image Spacing</em> is enabled, the gradient is calculated in the physical space; "
		"if it not enabled, the gradient is calculated in pixel space.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientMagnitudeRecursiveGaussianImageFilter.html\">"
		"Gradient Magnitude Recursive Gaussian Filter</a> in the ITK documentation.")
{
	addParameter("Normalize across scale", iAValueType::Boolean, true);
	addParameter("Sigma", iAValueType::Continuous, 1.0);
}

// iADerivative:

template<class T>
void derivative(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<float, DIM> RealImageType;
	typedef itk::DerivativeImageFilter< InputImageType, RealImageType > DIFType;

	auto derFilter = DIFType::New();
	derFilter->SetOrder(params["Order"].toUInt());
	derFilter->SetDirection(params["Direction"].toUInt());
	derFilter->SetInput( dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()) );
	filter->progress()->observe( derFilter );
	derFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(derFilter->GetOutput()));
}

void iADerivative::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(derivative, inputScalarType(), this, parameters);
}

iADerivative::iADerivative() :
	iAFilter("Derivative", "Gradient",
		"Computes the directional derivative for each image element.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DerivativeImageFilter.html\">"
		"Derivative Filter</a> in the ITK documentation.")
{
	addParameter("Order", iAValueType::Discrete, 1, 1);
	addParameter("Direction", iAValueType::Discrete, 0, 0, DIM-1);
}


#ifdef ITKHigherOrderGradient
// iAHigherOrderAccurateGradient

template<class T>
void hoaDerivative(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<double, DIM> OutputImageType;
	typedef itk::HigherOrderAccurateDerivativeImageFilter< InputImageType, OutputImageType > HOAGDFilter;

	auto hoaFilter = HOAGDFilter::New();
	hoaFilter->SetOrder(parameters["Order"].toUInt());
	hoaFilter->SetDirection(parameters["Direction"].toUInt());
	hoaFilter->SetOrderOfAccuracy(parameters["Order of Accuracy"].toUInt());
	hoaFilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe(hoaFilter);
	hoaFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(hoaFilter->GetOutput()));
}

void iAHigherOrderAccurateDerivative::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(hoaDerivative, inputScalarType(), this, parameters);
}

iAHigherOrderAccurateDerivative::iAHigherOrderAccurateDerivative() :
	iAFilter("Higher Order Accurate Derivative", "Gradient",
		"Computes the higher order accurate directional derivative of an image.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z)."
		"The approximation will be accurate to two times the <em>Order of Accuracy</em> in terms of Taylor series terms.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HigherOrderAccurateDerivativeImageFilter.html\">"
		"Higher Order Accurate Derivative Filter</a> in the ITK documentation.")
{
	addParameter("Order", iAValueType::Discrete, 1, 1);
	addParameter("Direction", iAValueType::Discrete, 0, 0, DIM-1);
	addParameter("Order of Accuracy", iAValueType::Discrete, 2, 1);
}

#endif

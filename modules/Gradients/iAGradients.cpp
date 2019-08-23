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
#include "iAGradients.h"

#include "itkHigerOrderAccurateGradient/itkHigherOrderAccurateDerivativeImageFilter.h"

#include <defines.h> // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkCastImageFilter.h>
#include <itkDerivativeImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkImageIOBase.h>

// iAGradientMagnitude

template<class T> void gradient_magnitude(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;
	typedef itk::GradientMagnitudeImageFilter< InputImageType, InputImageType > GMFType;

	auto gmFilter = GMFType::New();
	gmFilter->SetInput(dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()));
	gmFilter->SetUseImageSpacing(params["Use Image Spacing"].toBool());
	filter->progress()->observe(gmFilter);
	gmFilter->Update();
	filter->addOutput(gmFilter->GetOutput());
}

void iAGradientMagnitude::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(gradient_magnitude, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAGradientMagnitude)

iAGradientMagnitude::iAGradientMagnitude() :
	iAFilter("Gradient Magnitude", "Gradients",
		"Computes the gradient magnitude at each image element.<br/>"
		"If <em>Use Image Spacing</em> is enabled, the gradient is calculated in the physical space; "
		"if it not enabled, the gradient is calculated in pixel space.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientMagnitudeImageFilter.html\">"
		"Gradient Magnitude Filter</a> in the ITK documentation.")
{
	addParameter("Use Image Spacing", Boolean, true);
}

// iADerivative:

template<class T> 
void derivative(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<float, DIM> RealImageType;
	//typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	typedef itk::DerivativeImageFilter< InputImageType, RealImageType > DIFType;

	//auto toReal = CastToRealFilterType::New();
	//toReal->SetInput( dynamic_cast< InputImageType * >( image->itkImage() ) );
	auto derFilter = DIFType::New();
	derFilter->SetOrder(params["Order"].toUInt());
	derFilter->SetDirection(params["Direction"].toUInt());
	derFilter->SetInput( dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()) );
	filter->progress()->observe( derFilter );
	derFilter->Update();
	filter->addOutput(derFilter->GetOutput());
}

void iADerivative::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(derivative, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iADerivative)

iADerivative::iADerivative() :
	iAFilter("Derivative", "Gradients",
		"Computes the directional derivative for each image element.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DerivativeImageFilter.html\">"
		"Derivative Filter</a> in the ITK documentation.")
{
	addParameter("Order", Discrete, 1, 1);
	addParameter("Direction", Discrete, 0, 0, DIM-1);
}


// iAHigherOrderAccurateGradient

template<class T>
void hoa_derivative(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<double, DIM> OutputImageType;
	typedef itk::HigherOrderAccurateDerivativeImageFilter< InputImageType, OutputImageType > HOAGDFilter;

	auto hoaFilter = HOAGDFilter::New();
	hoaFilter->SetOrder(parameters["Order"].toUInt());
	hoaFilter->SetDirection(parameters["Direction"].toUInt());
	hoaFilter->SetOrderOfAccuracy(parameters["Order of Accuracy"].toUInt());
	hoaFilter->SetInput(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	filter->progress()->observe(hoaFilter);
	hoaFilter->Update();
	filter->addOutput(hoaFilter->GetOutput());
}
		
void iAHigherOrderAccurateDerivative::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(hoa_derivative, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAHigherOrderAccurateDerivative)

iAHigherOrderAccurateDerivative::iAHigherOrderAccurateDerivative() :
	iAFilter("Higher Order Accurate Derivative", "Gradients",
		"Computes the higher order accurate directional derivative of an image.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z)."
		"The approximation will be accurate to two times the <em>Order of Accuracy</em> in terms of Taylor series terms.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HigherOrderAccurateDerivativeImageFilter.html\">"
		"Higher Order Accurate Derivative Filter</a> in the ITK documentation.")
{
	addParameter("Order", Discrete, 1, 1);
	addParameter("Direction", Discrete, 0, 0, DIM-1);
	addParameter("Order of Accuracy", Discrete, 2, 1);
}

/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "iAGradients.h"

#include "defines.h" // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkDerivativeImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkImageIOBase.h>
#include <itkHigerOrderAccurateGradient/itkHigherOrderAccurateDerivativeImageFilter.h>



// iAGradientMagnitude

template<class T> void gradient_magnitude_template(iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;
	typedef itk::GradientMagnitudeImageFilter< InputImageType, InputImageType > GMFType;

	auto filter = GMFType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iAGradientMagnitude::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(gradient_magnitude_template, m_con->GetITKScalarPixelType(), m_progress, m_con);
}

IAFILTER_CREATE(iAGradientMagnitude)

iAGradientMagnitude::iAGradientMagnitude() :
	iAFilter("Gradient Magnitude", "Gradients",
		"Computes the gradient magnitude at each image element.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientMagnitudeImageFilter.html\">"
		"Gradient Magnitude Filter</a> in the ITK documentation.")
{	// no parameters
}

// iADerivative:

template<class T> 
void derivative_template( unsigned int o, unsigned int d, iAProgress* p, iAConnector* image )
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<float, DIM> RealImageType;
	//typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	typedef itk::DerivativeImageFilter< InputImageType, RealImageType > DIFType;

	//auto toReal = CastToRealFilterType::New();
	//toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	auto filter = DIFType::New();
	filter->SetOrder( o );
	filter->SetDirection( d );
	filter->SetInput( dynamic_cast< InputImageType * >(image->GetITKImage()) );
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iADerivative::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(derivative_template, m_con->GetITKScalarPixelType(),
		parameters["Order"].toUInt(), parameters["Direction"].toUInt(), m_progress, m_con);
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
	AddParameter("Order", Discrete, 1, 1);
	AddParameter("Direction", Discrete, 0, 0, DIM-1);
}


// iAHigherOrderAccurateGradient

template<class T>
void hoa_derivative_template(QMap<QString, QVariant> const & parameters, iAProgress* p, iAConnector* image)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<double, DIM> OutputImageType;
	typedef itk::HigherOrderAccurateDerivativeImageFilter< InputImageType, OutputImageType > HOAGDFilter;

	auto filter = HOAGDFilter::New();
	filter->SetOrder(parameters["Order"].toUInt());
	filter->SetDirection(parameters["Direction"].toUInt());
	filter->SetOrderOfAccuracy(parameters["Order of Accuracy"].toUInt());
	filter->SetInput(dynamic_cast<InputImageType *>(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}
		
void iAHigherOrderAccurateDerivative::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(hoa_derivative_template, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Order", Discrete, 1, 1);
	AddParameter("Direction", Discrete, 0, 0, DIM-1);
	AddParameter("Order of Accuracy", Discrete, 2, 1);
}

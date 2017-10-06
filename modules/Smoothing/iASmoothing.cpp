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
#include "iASmoothing.h"

#include "iAConnector.h"
#include "defines.h" // for DIM
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkBilateralImageFilter.h>


template<class T> 
void discrete_gaussian_template( double v, double me, bool outimg, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;
	typedef itk::CastImageFilter<InputImageType, RealImageType> CastToRealFilterType;
	typedef itk::DiscreteGaussianImageFilter<RealImageType, RealImageType > DGIFType;
	auto toReal = CastToRealFilterType::New();
	toReal->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	DGIFType::Pointer filter = DGIFType::New();
	filter->SetVariance(v);
	filter->SetMaximumError(me);
	filter->SetInput(toReal->GetOutput());
	filter->ReleaseDataFlagOn();
	if ( outimg )
	{
		typedef itk::CastImageFilter<RealImageType, InputImageType> CastToInputFilterType;
		typename CastToInputFilterType::Pointer toInput = CastToInputFilterType::New();
		toInput->SetInput( filter->GetOutput() );
		p->Observe( toInput );
		toInput->Update();
		image->SetImage( toInput->GetOutput() );
		toInput->ReleaseDataFlagOn();
	}
	else
	{
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
	}
	image->Modified();
}

IAFILTER_CREATE(iADiscreteGaussian)

void iADiscreteGaussian::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(discrete_gaussian_template, pixelType,
		parameters["Variance"].toDouble(),
		parameters["Maximum Error"].toDouble(),
		parameters["UShort Output"].toBool(),
		m_progress, m_con);
}

iADiscreteGaussian::iADiscreteGaussian() :
	iAFilter("Discrete Gaussian", "Smoothing/Blurring",
		"Performs a discrete gaussian blurring using the given <em>Variance</em> and <em>Maximum Error</em>.<br/>"
		"Note that the variance needs to be given in image coordinates (i.e. considering the spacing)."
		"If <em>UShort Output</em> is checked, then the output of the filter will be cast to an unsigned short image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DiscreteGaussianImageFilter.html\">"
		"Discrete Gaussian Filter</a> in the ITK documentation.")
{
	AddParameter("Variance", Continuous, 0);
	AddParameter("Maximum Error", Continuous, 0.01);
	AddParameter("UShort Output", Boolean, false);
}


template<class T>
void gradient_anisotropic_diffusion_template(unsigned int i, double t, double c, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::GradientAnisotropicDiffusionImageFilter< InputImageType, RealImageType > GADIFType;
	typename GADIFType::Pointer filter = GADIFType::New();
	filter->SetNumberOfIterations(i);
	filter->SetTimeStep(t);
	filter->SetConductanceParameter(c);
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAGradientAnisotropicDiffusion)

void iAGradientAnisotropicDiffusion::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(gradient_anisotropic_diffusion_template, pixelType,
		parameters["Number of Iterations"].toUInt(),
		parameters["Time Step"].toDouble(),
		parameters["Conductance"].toDouble(),
		m_progress, m_con);
}

iAGradientAnisotropicDiffusion::iAGradientAnisotropicDiffusion() :
	iAFilter("Gradient Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs a gradient anisotropic diffusion.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientAnisotropicDiffusionImageFilter.html\">"
		"Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	AddParameter("Number of Iterations", Discrete, 100, 1);
	AddParameter("Time Step", Continuous, 0.1);
	AddParameter("Conductance", Continuous, 0.1);
}


template<class T>
void curvature_anisotropic_diffusion_template(unsigned int i, double t, double c, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::CurvatureAnisotropicDiffusionImageFilter< InputImageType, RealImageType > CADIFType;
	typename CADIFType::Pointer filter = CADIFType::New();
	filter->SetNumberOfIterations(i);
	filter->SetTimeStep(t);
	filter->SetConductanceParameter(c);
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iACurvatureAnisotropicDiffusion)

void iACurvatureAnisotropicDiffusion::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(curvature_anisotropic_diffusion_template, pixelType,
		parameters["Number of Iterations"].toUInt(),
		parameters["Time Step"].toDouble(),
		parameters["Conductance"].toDouble(),
		m_progress, m_con);
}

iACurvatureAnisotropicDiffusion::iACurvatureAnisotropicDiffusion() :
	iAFilter("Curvature Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs an anisotropic diffusion using a modified curvature diffusion equation (MCDE)."
		"Note: The input to this filter needs to be float or double!<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CurvatureAnisotropicDiffusionImageFilter.html\">"
		"Curvature Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	AddParameter("Number of Iterations", Discrete, 100, 1);
	AddParameter("Time Step", Continuous, 0.1);
	AddParameter("Conductance", Continuous, 0.1);
}


template<class T>
void bilateral_template(double r, double d, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;
	typedef itk::BilateralImageFilter< InputImageType, RealImageType > BIFType;
	typename BIFType::Pointer filter = BIFType::New();
	filter->SetRangeSigma(r);
	filter->SetDomainSigma(d);
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iABilateral)

void iABilateral::Run(QMap<QString, QVariant> parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(bilateral_template, pixelType,
		parameters["Range Sigma"].toDouble(),
		parameters["Domain Sigma"].toDouble(),
		m_progress, m_con);
}

iABilateral::iABilateral() :
	iAFilter("Bilateral Image Filter", "Smoothing/Edge preserving smoothing",
		""
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BilateralImageFilter.html\">"
		"Bilateral Image Filter</a> in the ITK documentation.")
{
	AddParameter("Range Sigma", Continuous, 50);
	AddParameter("Domain Sigma", Continuous, 4);
}

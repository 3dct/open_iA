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
#include "iANoiseFilters.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkAdditiveGaussianNoiseImageFilter.h>
#include <itkSaltAndPepperNoiseImageFilter.h>
#include <itkShotNoiseImageFilter.h>
#include <itkSpeckleNoiseImageFilter.h>

template<class T> void additiveGaussianNoise_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::AdditiveGaussianNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto filter = NoiseFilterType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetMean(parameters["Mean"].toDouble());
	filter->SetStandardDeviation(parameters["Standard deviation"].toDouble());
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
}

void iAAdditiveGaussianNoise::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(additiveGaussianNoise_template, Input()[0]->GetITKScalarPixelType(), m_progress, Input()[0], parameters);
}

IAFILTER_CREATE(iAAdditiveGaussianNoise)

iAAdditiveGaussianNoise::iAAdditiveGaussianNoise() :
	iAFilter("Additive Gaussian", "Noise",
		"Adds additive gaussian white noise to an image.<br/>"
		"To each pixel intensity, a value from a normal distribution with the given "
		"<em>Mean</em> and <em>Standard deviation</em> is added.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AdditiveGaussianNoiseImageFilter.html\">"
		"Additive Gaussian Noise Filter</a> in the ITK documentation.")
{
	AddParameter("Mean", Continuous, 0);
	AddParameter("Standard deviation", Continuous, 0.1, std::numeric_limits<double>::epsilon());
}



template<class T> void saltAndPepperNoise_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::SaltAndPepperNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto filter = NoiseFilterType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetProbability(parameters["Probability"].toDouble());
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
}

void iASaltAndPepperNoise::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(saltAndPepperNoise_template, m_con->GetITKScalarPixelType(), m_progress, m_con, parameters);
}

IAFILTER_CREATE(iASaltAndPepperNoise)

iASaltAndPepperNoise::iASaltAndPepperNoise() :
	iAFilter("Salt and Pepper", "Noise",
		"Alter an image with fixed value impulse noise.<br/>"
		"Salt and pepper noise is a special kind of impulse noise where the value of the noise "
		"is either the maximum possible value in the image or its minimum.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SaltAndPepperNoiseImageFilter.html\">"
		"Salt and Pepper Noise Filter</a> in the ITK documentation.")
{
	AddParameter("Probability", Continuous, 0.1, 0, 1);
}



template<class T> void shotNoise_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::ShotNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto filter = NoiseFilterType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetScale(parameters["Scale"].toDouble());
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
}

void iAShotNoise::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(shotNoise_template, m_con->GetITKScalarPixelType(), m_progress, m_con, parameters);
}

IAFILTER_CREATE(iAShotNoise)

iAShotNoise::iAShotNoise() :
	iAFilter("Shot", "Noise",
		"Alter an image with shot noise.<br/>"
		"The shot noise follows a Poisson distribution with the pixel intensity as mean, "
		"scaled by the given <em>Scale</em> parameter.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ShotNoiseImageFilter.html\">"
		"Shot Noise Filter</a> in the ITK documentation.")
{
	AddParameter("Scale", Continuous, 1, std::numeric_limits<double>::epsilon());
}



template<class T> void speckleNoise_template(iAProgress* p, iAConnector* image, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::SpeckleNoiseImageFilter<InputImageType, InputImageType> NoiseFilterType;
	auto filter = NoiseFilterType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetStandardDeviation(parameters["Standard deviation"].toDouble());
	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();
}

void iASpeckleNoise::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(speckleNoise_template, m_con->GetITKScalarPixelType(), m_progress, m_con, parameters);
}

IAFILTER_CREATE(iASpeckleNoise)

iASpeckleNoise::iASpeckleNoise() :
	iAFilter("Speckle", "Noise",
		"Alter an image with speckle (multiplicative) noise.<br/>"
		"The speckle noise follows a gamma distribution of mean 1 and <em>Standard deviation</em>"
		"provided by the user. The noise is proportional to the pixel intensity.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SpeckleNoiseImageFilter.html\">"
		"Speckle Noise Filter</a> in the ITK documentation.")
{
	AddParameter("Standard deviation", Continuous, 0.1, std::numeric_limits<double>::epsilon());
}
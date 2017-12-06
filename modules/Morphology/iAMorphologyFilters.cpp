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
#include "iAMorphologyFilters.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkHessian3DToVesselnessMeasureImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>

template<class T> void dilation_template(iAProgress* p, iAConnector* image, int radius)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	typedef itk::GrayscaleDilateImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleDilateImageFilterType;

	StructuringElementType structuringElement;
	structuringElement.SetRadius(radius);
	structuringElement.CreateStructuringElement();
	auto dilateFilter = GrayscaleDilateImageFilterType::New();
	dilateFilter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	dilateFilter->SetKernel(structuringElement);
	p->Observe( dilateFilter );
	dilateFilter->Update(); 
	image->SetImage(dilateFilter->GetOutput());
	image->Modified();
	dilateFilter->ReleaseDataFlagOn();
}

void iADilation::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(dilation_template, m_con->GetITKScalarPixelType(),
		m_progress, m_con, parameters["Radius"].toInt());
}

IAFILTER_CREATE(iADilation)

iADilation::iADilation() :
	iAFilter("Dilation", "Morphology",
		"Dilate an image using grayscale morphology.<br/>"
		"Dilation takes the maximum of all the pixels identified by the "
		"structuring element (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleDilateImageFilter.html\">"
		"Grayscale Dilate Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation.")
{
	AddParameter("Radius", Discrete, 1, 1);
}



template<class T> void erosion_template(iAProgress* p, iAConnector* image, int radius)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	typedef itk::GrayscaleErodeImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleErodeImageFilterType;

	StructuringElementType structuringElement;
	structuringElement.SetRadius(radius);
	structuringElement.CreateStructuringElement();
	auto erodeFilter = GrayscaleErodeImageFilterType::New();
	erodeFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	erodeFilter->SetKernel(structuringElement);
	p->Observe( erodeFilter );
	erodeFilter->Update();
	image->SetImage(erodeFilter->GetOutput());
	image->Modified();
	erodeFilter->ReleaseDataFlagOn();
}

void iAErosion::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(erosion_template, m_con->GetITKScalarPixelType(),
		m_progress, m_con, parameters["Radius"].toInt());
}

IAFILTER_CREATE(iAErosion)

iAErosion::iAErosion() :
	iAFilter("Erosion", "Morphology",
		"Erodes an image using grayscale morphology.<br/>"
		"Erosion takes the maximum of all the pixels identified by the "
		"structuring element. (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleErodeImageFilter.html\">"
		"Grayscale Erode Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation.")
{
	AddParameter("Radius", Discrete, 1, 1);
}



template<class T> void vesselEnhancement_template(iAProgress* p, iAConnector* image, double sigma)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImageType::PixelType> EnhancementFilter; 
	typedef itk::HessianRecursiveGaussianImageFilter<InputImageType> HRGIFType;

	auto hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast< InputImageType * >( image->GetITKImage() ));
	hessfilter->SetSigma(sigma);
	hessfilter->Update();
	auto vesselness = EnhancementFilter::New();
	vesselness->SetInput( hessfilter->GetOutput() );
	vesselness->Update();
	image->SetImage(vesselness->GetOutput());
	image->Modified();
}

void iAVesselEnhancement::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(vesselEnhancement_template, m_con->GetITKScalarPixelType(),
		m_progress, m_con, parameters["Sigma"].toDouble());
}

IAFILTER_CREATE(iAVesselEnhancement)

iAVesselEnhancement::iAVesselEnhancement() :
	iAFilter("Vessel Enhancement", "Morphology",
		"Line filter to provide a vesselness measure for tubular objects from the Hessian matrix.<br/>"
		"<em>Sigma</em> is a parameter to the Hessian calculation via a "
		"recursive Gaussian filter, and is measured in units of image spacing.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HessianRecursiveGaussianImageFilter.html\">"
		"Hessian Recursive Gaussian Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1Hessian3DToVesselnessMeasureImageFilter.html\">"
		"Hessian 3D to Vesselness Measure Filter</a> in the ITK documentation.")
{
	AddParameter("Sigma", Continuous, 0);
}

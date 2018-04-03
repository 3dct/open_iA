/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

template<class T> void dilation(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	typedef itk::GrayscaleDilateImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleDilateImageFilterType;

	StructuringElementType structuringElement;
	structuringElement.SetRadius(params["Radius"].toUInt());
	structuringElement.CreateStructuringElement();
	auto dilateFilter = GrayscaleDilateImageFilterType::New();
	dilateFilter->SetInput(dynamic_cast< InputImageType * >(filter->Input()[0]->GetITKImage()));
	dilateFilter->SetKernel(structuringElement);
	filter->Progress()->Observe( dilateFilter );
	dilateFilter->Update(); 
	filter->AddOutput(dilateFilter->GetOutput());
}

void iADilation::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(dilation, InputPixelType(), this, parameters);
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



template<class T> void erosion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM> InputImageType;
	typedef itk::BinaryBallStructuringElement<typename InputImageType::PixelType,3> StructuringElementType;
	typedef itk::GrayscaleErodeImageFilter <InputImageType, InputImageType, StructuringElementType>
		GrayscaleErodeImageFilterType;

	StructuringElementType structuringElement;
	structuringElement.SetRadius(params["Radius"].toInt());
	structuringElement.CreateStructuringElement();
	auto erodeFilter = GrayscaleErodeImageFilterType::New();
	erodeFilter->SetInput( dynamic_cast< InputImageType * >( filter->Input()[0]->GetITKImage() ) );
	erodeFilter->SetKernel(structuringElement);
	filter->Progress()->Observe( erodeFilter );
	erodeFilter->Update();
	filter->AddOutput(erodeFilter->GetOutput());
}

void iAErosion::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(erosion, InputPixelType(), this, parameters);
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



template<class T> void vesselEnhancement(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImageType::PixelType> EnhancementFilter; 
	typedef itk::HessianRecursiveGaussianImageFilter<InputImageType> HRGIFType;

	auto hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast< InputImageType * >( filter->Input()[0]->GetITKImage() ));
	hessfilter->SetSigma(params["Sigma"].toDouble());
	hessfilter->Update();
	auto vesselness = EnhancementFilter::New();
	vesselness->SetInput( hessfilter->GetOutput() );
	vesselness->Update();
	filter->AddOutput(vesselness->GetOutput());
}

void iAVesselEnhancement::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(vesselEnhancement, InputPixelType(), this, parameters);
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

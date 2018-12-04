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

#include <itkBinaryMorphologicalClosingImageFilter.h>

namespace Morphology {
	struct morphEl {
		morphEl() {
			MorphOptions << "Ball" << "Box" << "Cross" << "Polygon" ;
		}
		
		QStringList MorphOptions;
		
	};
	
	const QString elem_type = "Structural ElementType";

	template<class T> 
	
	//template definitions; 
	using InputImageType = itk::Image< T, DIM>; 

	//alias for ball BallStructingElement
	template<class T>
	using BallElement = itk::BinaryBallStructuringElement<typename InputImageType<T>::PixelType, 3>;
	
	//alias for Flat Structing element; 
	template<class T>
	using FlatElement = itk::FlatStructuringElement<3>;

	const unsigned int PolyLines = 7; // default for polygon

	
}; 


template<class T> void dilation(iAFilter* filter, QMap<QString, QVariant> const & params)
{

	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();

	//default use ball

	if (str_Input.compare("Ball") == 0) {

		
		typedef itk::GrayscaleDilateImageFilter <InputImageType<T>, InputImageType<T>, BallElement<T>>
			GrayscaleDilateImageFilterType;

		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto dilateFilter = GrayscaleDilateImageFilterType::New();
		dilateFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		dilateFilter->SetKernel(structuringElement);
		filter->Progress()->Observe(dilateFilter);
		dilateFilter->Update();
		filter->AddOutput(dilateFilter->GetOutput());

	}else {
		typedef itk::GrayscaleDilateImageFilter <InputImageType<T>, InputImageType<T>, FlatElement<T>>
			GrayscaleDilateImageFilterType;

		//Create a box; 

		FlatElement<T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toInt());

		if (str_Input.compare("Box") == 0) {
			 structuringElement = FlatElement<T>::Box(elementRadius);
		}
		else	if (str_Input.compare("Cross") == 0) {
			structuringElement = FlatElement<T>::Cross(elementRadius);
		}
		else {
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);
		}


		auto dilateFilter = GrayscaleDilateImageFilterType::New();
		dilateFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		dilateFilter->SetKernel(structuringElement);


		filter->Progress()->Observe(dilateFilter);
		dilateFilter->Update();
		filter->AddOutput(dilateFilter->GetOutput());
	
	}
	
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
		"Default structuring element (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleDilateImageFilter.html\">"
		"Grayscale Dilate Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation.")
{
	AddParameter("Radius", Discrete, 1, 1);
	AddParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}



template<class T> void erosion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology; 
	std::string str_Input = params[elem_type].toString().toStdString(); 
	
	//default ball; 
	if (str_Input.compare("Ball") == 0) {
		
		typedef itk::GrayscaleErodeImageFilter <InputImageType<T>, InputImageType<T>, BallElement<T> >
			GrayscaleErodeImageFilterType;

		BallElement<T> structuringElement;
		
		structuringElement.SetRadius(params["Radius"].toInt());
		structuringElement.CreateStructuringElement();
		auto erodeFilter = GrayscaleErodeImageFilterType::New();
		erodeFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		erodeFilter->SetKernel(structuringElement);
		
		filter->Progress()->Observe(erodeFilter);
		erodeFilter->Update();
		filter->AddOutput(erodeFilter->GetOutput());
	}
	else {


		typedef itk::GrayscaleErodeImageFilter <InputImageType<T>, InputImageType<T>, FlatElement<T> /*StructuringElementType*/>
			GrayscaleErodeImageFilterType;

		//Create a box; 

		FlatElement <T> structuringElement; 
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toInt());

		if (str_Input.compare("Box") == 0) {
			 structuringElement = FlatElement<T>::Box(elementRadius);
		}else	if (str_Input.compare("Cross") == 0) {
			structuringElement = FlatElement<T>::Cross(elementRadius);
		}
		else {
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);
		}


		auto erodeFilter = GrayscaleErodeImageFilterType::New();
		erodeFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		erodeFilter->SetKernel(structuringElement);


		filter->Progress()->Observe(erodeFilter);
		erodeFilter->Update();
		filter->AddOutput(erodeFilter->GetOutput());
	
	}
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
		"Default structuring element. (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleErodeImageFilter.html\">"
		"Grayscale Erode Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation."
		"Other Structuring Elements are Box, Cross and Polygon referred as FlatStructuringElement. For further information see"
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
	)
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

#include <itkBinaryMorphologicalOpeningImageFilter.h>
//closing
template<class T> void opening(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();

	//default ball; 
	if (str_Input.compare("Ball") == 0) {

		
		typedef itk::BinaryMorphologicalOpeningImageFilter<InputImageType<T>, InputImageType<T>, BallElement<T>>
			GrayscaleOpeningImageFilterType;

		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto openingFilter = GrayscaleOpeningImageFilterType::New(); //::New();
		openingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		openingFilter->SetKernel(structuringElement);
		filter->Progress()->Observe(openingFilter);
		openingFilter->Update();
		filter->AddOutput(openingFilter->GetOutput());
	}else {
		
		typedef itk::BinaryMorphologicalOpeningImageFilter<InputImageType<T>, InputImageType<T>, FlatElement<T>>
			GrayscaleOpeningImageFilterType;

		//Create a box;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toInt());

		if (str_Input.compare("Box") == 0) {
			 structuringElement = FlatElement<T>::Box(elementRadius);
		}
		else	if (str_Input.compare("Cross") == 0) {
			structuringElement = FlatElement<T>::Cross(elementRadius);
		}
		else {
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);
		}


		auto openingFilter = GrayscaleOpeningImageFilterType::New();
		openingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		openingFilter->SetKernel(structuringElement);


		filter->Progress()->Observe(openingFilter);
		openingFilter->Update();
		filter->AddOutput(openingFilter->GetOutput());
	
	}
}

void iAOpening::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(opening, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAOpening)


//TBD Description anpassen


//TBD Description anpassen
iAOpening::iAOpening():
	iAFilter("Opening", "Morphology",
		"Opening an image using grayscale morphology.<br/>"
		"Opening takes the maximum of all the pixels identified by the "
		"Default structuring element (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleDilateImageFilter.html\">"
		"Grayscale Dilate Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation."
		"Other Structuring Elements are Box, Cross and Polygon referred as FlatStructuringElement. For further information see"
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
	)
{
	Morphology::morphEl morph_text;
	AddParameter("Radius", Discrete, 1, 1);
	AddParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

//closing

#include <itkBinaryMorphologicalOpeningImageFilter.h>
//closing
template<class T> void closing(iAFilter* filter, QMap<QString, QVariant> const & params)
{

	using namespace Morphology; 
	std::string str_Input = params[elem_type].toString().toStdString();
	if (str_Input.compare("Ball") == 0) {
		
		typedef itk::BinaryMorphologicalClosingImageFilter<InputImageType<T>, InputImageType<T>, BallElement<T>>
			GrayscaleClosingImageFilterType;

		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto openingFilter = GrayscaleClosingImageFilterType::New(); 
		openingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		openingFilter->SetKernel(structuringElement);
		filter->Progress()->Observe(openingFilter);
		openingFilter->Update();
		filter->AddOutput(openingFilter->GetOutput());
	}else {
		typedef itk::BinaryMorphologicalClosingImageFilter<InputImageType<T>, InputImageType<T>, FlatElement<T>>
			GrayscaleClosingImageFilterType;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toInt());

		if (str_Input.compare("Box") == 0) {
			 structuringElement = FlatElement<T>::Box(elementRadius);
		}
		else	if (str_Input.compare("Cross") == 0) {
			structuringElement = FlatElement<T>::Cross(elementRadius);
		}
		else {
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);
		}


		auto openingFilter = GrayscaleClosingImageFilterType::New();
		openingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->Input()[0]->GetITKImage()));
		openingFilter->SetKernel(structuringElement);
	
	}
}

void iAClosing::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(closing, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAClosing)


//TBD Description anpassen
iAClosing::iAClosing() :
	iAFilter("Closing", "Morphology",
		"Closing an image using grayscale morphology.<br/>"
		"Closing takes the maximum of all the pixels identified by the "
		"Default structuring element (a ball with the given <em>Radius</em> in all directions).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleDilateImageFilter.html\">"
		"Grayscale Dilate Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a> in the ITK documentation."
		"Other Structuring Elements are Box, Cross and Polygon referred as FlatStructuringElement. For further information see"
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
	)
{
	Morphology::morphEl morph_text;
	AddParameter("Radius", Discrete, 1, 1);
	AddParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

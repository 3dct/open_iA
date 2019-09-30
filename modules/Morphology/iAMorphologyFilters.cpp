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
#include "iAMorphologyFilters.h"

#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAConsole.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkGrayscaleFillholeImageFilter.h>
#include <itkGrayscaleMorphologicalClosingImageFilter.h>
#include <itkGrayscaleMorphologicalOpeningImageFilter.h>
#include <itkHessian3DToVesselnessMeasureImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkOpeningByReconstructionImageFilter.h>

// NOTE: The 'binary' versions of the dilation (e.g., itkBinaryDilateImageFilter), erode, fill hole, opening,
// and closing filters have been replaced by the 'grayscale' versions of these filters 
// (e.g., itkGrayscaleDilateImageFilter), because of convince (data types) and performance (parallelization).

namespace Morphology
{
	struct morphEl
	{
		morphEl()
		{
			MorphOptions << "Ball" << "Box" << "Cross" << "Polygon" ;
		}
		QStringList MorphOptions;
	};

	const QString elem_type = "Structuring Element";

	//template definitions;
	template<class T>
	using InputImageType = itk::Image< T, DIM>;

	//alias for ball BallStructingElement
	template<class T>
	using BallElement = itk::BinaryBallStructuringElement<typename InputImageType<T>::PixelType, DIM>;

	//alias for Flat Structing element;
	template<class T>
	using FlatElement = itk::FlatStructuringElement<DIM>;

	const unsigned int PolyLines = 7; // default for polygon
}


template<class T> void dilation(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();

	if (str_Input.compare("Ball") == 0)
	{
		typedef itk::GrayscaleDilateImageFilter <InputImageType<T>, InputImageType<T>, BallElement<T>>
			DilateImageFilterType;
		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto dilateFilter = DilateImageFilterType::New();
		dilateFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		dilateFilter->SetKernel(structuringElement);
		filter->progress()->observe(dilateFilter);
		dilateFilter->Update();
		filter->addOutput(dilateFilter->GetOutput());
	}
	else
	{
		typedef itk::GrayscaleDilateImageFilter <InputImageType<T>, InputImageType<T>, FlatElement<T>>
			DilateImageFilterType;

		FlatElement<T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toUInt());
		if (str_Input.compare("Box") == 0)
			 structuringElement = FlatElement<T>::Box(elementRadius);
		else if (str_Input.compare("Cross") == 0)
			structuringElement = FlatElement<T>::Cross(elementRadius);
		else
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);

		auto dilateFilter = DilateImageFilterType::New();
		dilateFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		dilateFilter->SetKernel(structuringElement);
		filter->progress()->observe(dilateFilter);
		dilateFilter->Update();
		filter->addOutput(dilateFilter->GetOutput());
	}
}

void iADilation::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(dilation, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iADilation)

iADilation::iADilation() :
	iAFilter("Dilation", "Morphology",
		"Dilate an image using grayscale morphology.<br/>"
		"Dilation takes the maximum of all the pixels identified by the chosen "
		"<em>Structuring Element</em>, a ball, box, cross or polygon with the given <em>Radius</em> in all directions.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleDilateImageFilter.html\">"
		"Grayscale Dilate Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	Morphology::morphEl morph_text;
	addParameter("Radius", Discrete, 1, 1);
	addParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

template<class T> void erosion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();

	if (str_Input.compare("Ball") == 0)
	{
		typedef itk::GrayscaleErodeImageFilter <InputImageType<T>, InputImageType<T>, BallElement<T> >
			ErodeImageFilterType;

		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto erodeFilter = ErodeImageFilterType::New();
		erodeFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		erodeFilter->SetKernel(structuringElement);
		filter->progress()->observe(erodeFilter);
		erodeFilter->Update();
		filter->addOutput(erodeFilter->GetOutput());
	}
	else
	{
		typedef itk::GrayscaleErodeImageFilter <InputImageType<T>, InputImageType<T>, FlatElement<T> /*StructuringElementType*/>
			ErodeImageFilterType;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toUInt());

		if (str_Input.compare("Box") == 0)
			 structuringElement = FlatElement<T>::Box(elementRadius);
		else if (str_Input.compare("Cross") == 0)
			structuringElement = FlatElement<T>::Cross(elementRadius);
		else
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);

		auto erodeFilter = ErodeImageFilterType::New();
		erodeFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		erodeFilter->SetKernel(structuringElement);
		filter->progress()->observe(erodeFilter);
		erodeFilter->Update();
		filter->addOutput(erodeFilter->GetOutput());
	}
}

void iAErosion::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(erosion, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAErosion)

iAErosion::iAErosion() :
	iAFilter("Erosion", "Morphology",
		"Erodes an image using grayscale morphology.<br/>"
		"Erosion takes the maximum of all the pixels identified by the given "
		"<em>Sructuring Element</em>, a ball, box, cross or polygon with the given <em>Radius</em> in all directions.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleErodeImageFilter.html\">"
		"Grayscale Erode Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	Morphology::morphEl morph_text;
	addParameter("Radius", Discrete, 1, 1);
	addParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

template<class T> void vesselEnhancement(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImageType::PixelType> EnhancementFilter;
	typedef itk::HessianRecursiveGaussianImageFilter<InputImageType> HRGIFType;

	auto hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast< InputImageType * >( filter->input()[0]->itkImage() ));
	hessfilter->SetSigma(params["Sigma"].toDouble());
	hessfilter->Update();
	auto vesselness = EnhancementFilter::New();
	vesselness->SetInput( hessfilter->GetOutput() );
	vesselness->Update();
	filter->addOutput(vesselness->GetOutput());
}

void iAVesselEnhancement::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(vesselEnhancement, inputPixelType(), this, parameters);
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
	addParameter("Sigma", Continuous, 0);
}

template<class T> void morphOpening(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();

	if (str_Input.compare("Ball") == 0)
	{
		typedef itk::GrayscaleMorphologicalOpeningImageFilter<InputImageType<T>, InputImageType<T>, BallElement<T>>
			MorphOpeningImageFilterType;
		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto morphOpeningFilter = MorphOpeningImageFilterType::New(); //::New();
		morphOpeningFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		morphOpeningFilter->SetKernel(structuringElement);
		filter->progress()->observe(morphOpeningFilter);
		morphOpeningFilter->Update();
		filter->addOutput(morphOpeningFilter->GetOutput());
	}
	else
	{
		typedef itk::GrayscaleMorphologicalOpeningImageFilter<InputImageType<T>, InputImageType<T>, FlatElement<T>>
			MorphOpeningImageFilterType;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toUInt());
		if (str_Input.compare("Box") == 0)
			 structuringElement = FlatElement<T>::Box(elementRadius);
		else if (str_Input.compare("Cross") == 0)
			structuringElement = FlatElement<T>::Cross(elementRadius);
		else
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);

		auto morphOpeningFilter = MorphOpeningImageFilterType::New();
		morphOpeningFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		morphOpeningFilter->SetKernel(structuringElement);
		filter->progress()->observe(morphOpeningFilter);
		morphOpeningFilter->Update();
		filter->addOutput(morphOpeningFilter->GetOutput());
	}
}

void iAMorphOpening::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(morphOpening, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAMorphOpening)

iAMorphOpening::iAMorphOpening():
	iAFilter("Morphological Opening", "Morphology",
		"The morphological opening of an image 'f' is defined as: Opening(f) = Dilatation(Erosion(f)).<br/>"
		"The structuring element is assumed to be composed of binary values (zero or one). Only elements of the "
		"structuring element having values > 0 are candidates for affecting the center pixel.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleMorphologicalOpeningImageFilter.html\">"
		"Grayscale Morphological Opening Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	Morphology::morphEl morph_text;
	addParameter("Radius", Discrete, 1, 1);
	addParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

template<class T> void morphClosing(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();
	if (str_Input.compare("Ball") == 0)
	{
		typedef itk::GrayscaleMorphologicalClosingImageFilter<InputImageType<T>, InputImageType<T>, BallElement<T>>
			MorphClosingImageFilterType;
		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto morphClosingFilter = MorphClosingImageFilterType::New();
		morphClosingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		morphClosingFilter->SetKernel(structuringElement);
		filter->progress()->observe(morphClosingFilter);
		morphClosingFilter->Update();
		filter->addOutput(morphClosingFilter->GetOutput());
	}
	else
	{
		typedef itk::GrayscaleMorphologicalClosingImageFilter<InputImageType<T>, InputImageType<T>, FlatElement<T>>
			MorphClosingImageFilterType;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toUInt());
		if (str_Input.compare("Box") == 0)
			 structuringElement = FlatElement<T>::Box(elementRadius);
		else if (str_Input.compare("Cross") == 0)
			structuringElement = FlatElement<T>::Cross(elementRadius);
		else
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);

		auto morphClosingFilter = MorphClosingImageFilterType::New();
		morphClosingFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
		morphClosingFilter->SetKernel(structuringElement);
		filter->progress()->observe(morphClosingFilter);
		morphClosingFilter->Update();
		filter->addOutput(morphClosingFilter->GetOutput());
	}
}

void iAMorphClosing::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(morphClosing, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAMorphClosing)

iAMorphClosing::iAMorphClosing() :
	iAFilter("Morphological Closing", "Morphology",
		"The morphological closing of an image 'f' is defined as: Closing(f) = Erosion(Dilation(f)).<br/>"
		"The structuring element is assumed to be composed of binary values (zero or one). Only elements of the "
		"structuring element having values > 0 are candidates for affecting the center pixel.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleMorphologicalClosingImageFilter.html\">"
		"Grayscale Morphological Closing Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	Morphology::morphEl morph_text;
	addParameter("Radius", Discrete, 1, 1);
	addParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

template<class T> void fillHole(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	using namespace Morphology;
	typedef itk::GrayscaleFillholeImageFilter <InputImageType<T>, InputImageType<T>> FillHoleImageFilterType;
	typename FillHoleImageFilterType::Pointer fillHoleFilter = FillHoleImageFilterType::New();
	fillHoleFilter->SetInput(dynamic_cast<InputImageType<T> *>(filter->input()[0]->itkImage()));
	fillHoleFilter->SetFullyConnected(params["Fully Connected"].toBool());
	filter->progress()->observe(fillHoleFilter);
	fillHoleFilter->Update();
	filter->addOutput(fillHoleFilter->GetOutput());
}

void iAFillHole::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(fillHole, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAFillHole)

iAFillHole::iAFillHole() :
	iAFilter("Fill Hole", "Morphology",
		"Remove local minima not connected to the boundary of the image.<br/>"
		"GrayscaleFillholeImageFilter fills holes in a grayscale image. "
		"Holes are local minima in the grayscale topography that are not connected "
		"to boundaries of the image. Gray level values adjacent to a hole are extrapolated across the hole.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleFillholeImageFilter.html\">"
		"GrayscaleFillholeImageFilter</a>")
{
	addParameter("Fully Connected", Boolean, false);
}

template<class T> void openingByReconstruction(iAFilter* filter, QMap<QString, QVariant> const& params)
{
	using namespace Morphology;
	std::string str_Input = params[elem_type].toString().toStdString();
	if (str_Input.compare("Ball") == 0)
	{
		typedef itk::OpeningByReconstructionImageFilter<InputImageType<T>, InputImageType<T>, BallElement<T>>
			OpeningByReconstructionImageFilterFilterType;
		BallElement<T> structuringElement;
		structuringElement.SetRadius(params["Radius"].toUInt());
		structuringElement.CreateStructuringElement();
		auto OpeningByReconstruction = OpeningByReconstructionImageFilterFilterType::New();
		OpeningByReconstruction->SetInput(dynamic_cast<InputImageType<T>*>(filter->input()[0]->itkImage()));
		OpeningByReconstruction->SetKernel(structuringElement);
		filter->progress()->observe(OpeningByReconstruction);
		OpeningByReconstruction->Update();
		filter->addOutput(OpeningByReconstruction->GetOutput());
	}
	else
	{
		typedef itk::OpeningByReconstructionImageFilter<InputImageType<T>, InputImageType<T>, FlatElement<T>>
			OpeningByReconstructionImageFilterType;

		FlatElement <T> structuringElement;
		typename FlatElement<T>::RadiusType elementRadius;
		elementRadius.Fill(params["Radius"].toUInt());
		if (str_Input.compare("Box") == 0)
			structuringElement = FlatElement<T>::Box(elementRadius);
		else if (str_Input.compare("Cross") == 0)
			structuringElement = FlatElement<T>::Cross(elementRadius);
		else
			structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);

		auto OpeningByReconstruction = OpeningByReconstructionImageFilterType::New();
		OpeningByReconstruction->SetInput(dynamic_cast<InputImageType<T>*>(filter->input()[0]->itkImage()));
		OpeningByReconstruction->SetKernel(structuringElement);
		filter->progress()->observe(OpeningByReconstruction);
		OpeningByReconstruction->Update();
		filter->addOutput(OpeningByReconstruction->GetOutput());
	}
}

void iAOpeningByReconstruction::performWork(QMap<QString, QVariant> const& parameters)
{
	ITK_TYPED_CALL(openingByReconstruction, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAOpeningByReconstruction)

iAOpeningByReconstruction::iAOpeningByReconstruction() :
	iAFilter("OpeningByReconstructionImageFilter", "Morphology",
		"This filter preserves regions, in the foreground, that can completely contain the structuring element. <br/>"
		"At the same time, this filter eliminates all other regions of foreground pixels. <br/>"
		"Contrary to the morphological opening, the opening by reconstruction preserves the shape of <br/>"
		"the components that are not removed by erosion. The opening by reconstruction of an image <f> is defined as:<br/>"
		"OpeningByReconstruction(f) = DilationByRecontruction(f, Erosion(f)).<br/><br/> "

		"Opening by reconstruction not only removes structures destroyed by the erosion,<br/> "
		"but also levels down the contrast of the brightest regions.If PreserveIntensities is on, a <br/>"
		"subsequent reconstruction by dilation using a marker image that is the original image for all unaffected pixels. <br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OpeningByReconstructionImageFilter.html\">"
		"OpeningByReconstructionImageFilter </a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	Morphology::morphEl morph_text;
	addParameter("Radius", Discrete, 1, 1);
	addParameter(Morphology::elem_type, Categorical, morph_text.MorphOptions);
}

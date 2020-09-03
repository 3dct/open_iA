/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#include <itkBinaryBallStructuringElement.h>
#include <itkGrayscaleDilateImageFilter.h>
#include <itkGrayscaleErodeImageFilter.h>
#include <itkGrayscaleFillholeImageFilter.h>
#include <itkGrayscaleMorphologicalClosingImageFilter.h>
#include <itkGrayscaleMorphologicalOpeningImageFilter.h>
#include <itkHessian3DToVesselnessMeasureImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkOpeningByReconstructionImageFilter.h>
#include <itkClosingByReconstructionImageFilter.h>
#pragma GCC diagnostic pop

// NOTE: The 'binary' versions of the dilation (e.g., itkBinaryDilateImageFilter), erode, fill hole, opening,
// and closing filters have been replaced by the 'grayscale' versions of these filters
// (e.g., itkGrayscaleDilateImageFilter), because of convenience (more data types supported) and performance (parallelization).

namespace
{
	QStringList structuringElementNames()
	{
		return QStringList() << "Ball" << "Box" << "Cross" << "Polygon";
	}

	const QString StructuringElementParamName = "Structuring Element";

	template<class T> using InputImage = itk::Image<T, DIM>;
	template<class T> using BallElement = itk::BinaryBallStructuringElement<typename InputImage<T>::PixelType, DIM>;
	template<class T> using FlatElement = itk::FlatStructuringElement<DIM>;

	const unsigned int PolyLines = 7; // default for polygon
}

template<class MorphOp, class T> void morphOp(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	QString strElemName = params[StructuringElementParamName].toString();
	FlatElement<T> structuringElement;
	typename FlatElement<T>::RadiusType elementRadius;
	elementRadius.Fill(params["Radius"].toUInt());
	if (strElemName == "Box")
	{
		structuringElement = FlatElement<T>::Box(elementRadius);
	}
	else if (strElemName == "Cross")
	{
		structuringElement = FlatElement<T>::Cross(elementRadius);
	}
	else if (strElemName == "Ball")
	{
		structuringElement = FlatElement<T>::Ball(elementRadius);
	}
	else // Polygon
	{
		structuringElement = FlatElement<T>::Polygon(elementRadius, PolyLines);
	}

	auto morphOpFilter = MorphOp::New();
	morphOpFilter->SetInput(dynamic_cast<InputImage<T> *>(filter->input()[0]->itkImage()));
	morphOpFilter->SetKernel(structuringElement);
	filter->progress()->observe(morphOpFilter);
	morphOpFilter->Update();
	filter->addOutput(morphOpFilter->GetOutput());
}


template<class T> void dilation(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::GrayscaleDilateImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
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
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void erosion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::GrayscaleErodeImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
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
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void morphOpening(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::GrayscaleMorphologicalOpeningImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAMorphOpening::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(morphOpening, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAMorphOpening)

iAMorphOpening::iAMorphOpening():
	iAFilter("Opening", "Morphology",
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
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void morphClosing(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::GrayscaleMorphologicalClosingImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAMorphClosing::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(morphClosing, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAMorphClosing)

iAMorphClosing::iAMorphClosing() :
	iAFilter("Closing", "Morphology",
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
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void openingByReconstruction(iAFilter* filter, QMap<QString, QVariant> const& params)
{
	typedef itk::OpeningByReconstructionImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAOpeningByReconstruction::performWork(QMap<QString, QVariant> const& parameters)
{
	ITK_TYPED_CALL(openingByReconstruction, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAOpeningByReconstruction)

iAOpeningByReconstruction::iAOpeningByReconstruction() :
	iAFilter("Opening by Reconstruction", "Morphology",
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
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void closingByReconstruction(iAFilter* filter, QMap<QString, QVariant> const& params)
{
	typedef itk::ClosingByReconstructionImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAClosingByReconstruction::performWork(QMap<QString, QVariant> const& parameters)
{
	ITK_TYPED_CALL(closingByReconstruction, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAClosingByReconstruction)

iAClosingByReconstruction::iAClosingByReconstruction() :
	iAFilter("Closing by Reconstruction", "Morphology",
		"TThis filter is similar to the morphological closing, but contrary to the morphological closing, <br/>"
		"the closing by reconstruction preserves the shape of the components. <br/>"
		"The closing by reconstruction of an image <f> is defined as:"
		"ClosingByReconstruction(f) = ErosionByReconstruction(f, Dilation(f)).<br/><br/> "

		"losing by reconstruction not only preserves structures preserved by the dilation, <br/>"
		"but also levels raises the contrast of the darkest regions. If PreserveIntensities <br/>"
		"is on, a subsequent reconstruction by dilation using a marker image that is the original image for all unaffected pixels<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OpeningByReconstructionImageFilter.html\">"
		"ClosingByReconstructionImageFilter </a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryBallStructuringElement.html\">"
		"Binary Ball Structuring Element</a>, as well as the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlatStructuringElement.html\">"
		"FlatStructuringElement (Box, Cross and Polygon)</a> "
		"in the ITK documentation.")
{
	addParameter("Radius", Discrete, 1, 1);
	addParameter(StructuringElementParamName, Categorical, structuringElementNames());
}



template<class T> void fillHole(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::GrayscaleFillholeImageFilter <InputImage<T>, InputImage<T>> FillHoleImageFilterType;
	typename FillHoleImageFilterType::Pointer fillHoleFilter = FillHoleImageFilterType::New();
	fillHoleFilter->SetInput(dynamic_cast<InputImage<T> *>(filter->input()[0]->itkImage()));
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



template<class T> void vesselEnhancement(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImage<T>::PixelType> EnhancementFilter;
	typedef itk::HessianRecursiveGaussianImageFilter<InputImage<T>> HRGIFType;

	auto hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast<InputImage<T> *>(filter->input()[0]->itkImage()));
	hessfilter->SetSigma(params["Sigma"].toDouble());
	hessfilter->Update();
	auto vesselness = EnhancementFilter::New();
	vesselness->SetInput(hessfilter->GetOutput());
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

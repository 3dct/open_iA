// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>    // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryFillholeImageFilter.h>
#include <itkBinaryThinningImageFilter.h>
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

IAFILTER_DEFAULT_CLASS(iADilation);
IAFILTER_DEFAULT_CLASS(iAErosion);
IAFILTER_DEFAULT_CLASS(iAMorphOpening);
IAFILTER_DEFAULT_CLASS(iAMorphClosing);
IAFILTER_DEFAULT_CLASS(iAOpeningByReconstruction);
IAFILTER_DEFAULT_CLASS(iAClosingByReconstruction);

IAFILTER_DEFAULT_CLASS(iABinaryThinning);
IAFILTER_DEFAULT_CLASS(iABinaryFillHole);
IAFILTER_DEFAULT_CLASS(iAGrayscaleFillHole);
IAFILTER_DEFAULT_CLASS(iAVesselEnhancement);

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

template<class MorphOp, class T> void morphOp(iAFilter* filter, QVariantMap const & params)
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
	morphOpFilter->SetInput(dynamic_cast<InputImage<T> *>(filter->imageInput(0)->itkImage()));
	morphOpFilter->SetKernel(structuringElement);
	filter->progress()->observe(morphOpFilter);
	morphOpFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(morphOpFilter->GetOutput()));
}


template<class T> void dilation(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::GrayscaleDilateImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iADilation::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(dilation, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template<class T> void erosion(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::GrayscaleErodeImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAErosion::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(erosion, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template<class T> void morphOpening(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::GrayscaleMorphologicalOpeningImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAMorphOpening::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(morphOpening, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template<class T> void morphClosing(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::GrayscaleMorphologicalClosingImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAMorphClosing::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(morphClosing, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template<class T> void openingByReconstruction(iAFilter* filter, QVariantMap const& params)
{
	typedef itk::OpeningByReconstructionImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAOpeningByReconstruction::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(openingByReconstruction, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template<class T> void closingByReconstruction(iAFilter* filter, QVariantMap const& params)
{
	typedef itk::ClosingByReconstructionImageFilter<InputImage<T>, InputImage<T>, FlatElement<T>> MorphOpType;
	morphOp<MorphOpType, T>(filter, params);
}

void iAClosingByReconstruction::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(closingByReconstruction, inputScalarType(), this, parameters);
}

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
	addParameter("Radius", iAValueType::Discrete, 1, 1);
	addParameter(StructuringElementParamName, iAValueType::Categorical, structuringElementNames());
}



template <class T>
void binaryThinning(iAFilter* filter, QVariantMap const& params)
{
	Q_UNUSED(params);
	typedef itk::BinaryThinningImageFilter<InputImage<T>, InputImage<T>> BinaryThinningFilterType;
	auto thinningFilter = BinaryThinningFilterType::New();
	thinningFilter->SetInput(dynamic_cast<InputImage<T>*>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe(thinningFilter);
	thinningFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(thinningFilter->GetOutput()));
}

void iABinaryThinning::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(binaryThinning, inputScalarType(), this, parameters);
}

iABinaryThinning::iABinaryThinning() :
	iAFilter("Binary thinning", "Morphology",
		"Computes one-pixel-wide edges of the input image.<br/>"
		"The input is assumed to be a binary image. If the foreground pixels of the input image "
		"do not have a value of 1, they are rescaled to 1 internally to simplify the computation. "
		"The filter will produce a skeleton of the object.The output background values are 0, "
		"and the foreground values are 1. This filter is a sequential thinning algorithm and "
		"known to be computational time dependable on the image	size.<br />"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryThinningImageFilter.html\">"
		"Binary Thinning Image Filter</a> in the ITK documentation.")
{
}



template<class T> void binaryFillHole(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::BinaryFillholeImageFilter<InputImage<T>> FillHoleImageFilterType;
	auto fillHoleFilter = FillHoleImageFilterType::New();
	fillHoleFilter->SetInput(dynamic_cast<InputImage<T> *>(filter->imageInput(0)->itkImage()));
	fillHoleFilter->SetFullyConnected(params["Fully Connected"].toBool());
	fillHoleFilter->SetForegroundValue(params["Foreground Value"].toDouble());
	filter->progress()->observe(fillHoleFilter);
	fillHoleFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(fillHoleFilter->GetOutput()));
}

void iABinaryFillHole::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(binaryFillHole, inputScalarType(), this, parameters);
}

iABinaryFillHole::iABinaryFillHole() :
	iAFilter("Fill Hole (binary)", "Morphology",
		"Remove holes not connected to the boundary of the image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryFillholeImageFilter.html\">"
		"binary fill hole image filter</a> in the ITK documentation")
{
	addParameter("Fully Connected", iAValueType::Boolean, false);
	addParameter("Foreground Value", iAValueType::Continuous, 1);
}



template <class T>
void grayscaleFillHole(iAFilter* filter, QVariantMap const& params)
{
	typedef itk::GrayscaleFillholeImageFilter<InputImage<T>, InputImage<T>> FillHoleImageFilterType;
	auto fillHoleFilter = FillHoleImageFilterType::New();
	fillHoleFilter->SetInput(dynamic_cast<InputImage<T>*>(filter->imageInput(0)->itkImage()));
	fillHoleFilter->SetFullyConnected(params["Fully Connected"].toBool());
	filter->progress()->observe(fillHoleFilter);
	fillHoleFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(fillHoleFilter->GetOutput()));
}

void iAGrayscaleFillHole::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(grayscaleFillHole, inputScalarType(), this, parameters);
}

iAGrayscaleFillHole::iAGrayscaleFillHole() :
	iAFilter("Fill Hole (grayscale)", "Morphology",
		"Remove local minima not connected to the boundary of the image.<br/>"
		"GrayscaleFillholeImageFilter fills holes in a grayscale image. "
		"Holes are local minima in the grayscale topography that are not connected "
		"to boundaries of the image. Gray level values adjacent to a hole are extrapolated across the hole.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GrayscaleFillholeImageFilter.html\">"
		"GrayscaleFillholeImageFilter</a>")
{
	addParameter("Fully Connected", iAValueType::Boolean, false);
}



template<class T> void vesselEnhancement(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Hessian3DToVesselnessMeasureImageFilter<typename InputImage<T>::PixelType> EnhancementFilter;
	typedef itk::HessianRecursiveGaussianImageFilter<InputImage<T>> HRGIFType;

	auto hessfilter = HRGIFType::New();
	hessfilter->SetInput(dynamic_cast<InputImage<T> *>(filter->imageInput(0)->itkImage()));
	hessfilter->SetSigma(params["Sigma"].toDouble());
	hessfilter->Update();
	auto vesselness = EnhancementFilter::New();
	vesselness->SetInput(hessfilter->GetOutput());
	vesselness->Update();
	filter->addOutput(std::make_shared<iAImageData>(vesselness->GetOutput()));
}

void iAVesselEnhancement::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(vesselEnhancement, inputScalarType(), this, parameters);
}

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
	addParameter("Sigma", iAValueType::Continuous, 0);
}

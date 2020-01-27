/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAGeometricTransformations.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>
#include <mdichild.h>

#include <itkBSplineInterpolateImageFunction.h>
#include <itkChangeInformationImageFilter.h>
#include <itkConstantPadImageFilter.h>
#include <itkImageIOBase.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkWindowedSincInterpolateImageFunction.h>

#include <vtkImageData.h>

namespace
{
	const QString InterpLinear("Linear");
	const QString InterpNearestNeighbour("Nearest Neighbour");
	const QString InterpBSpline("BSpline");
	const QString InterpWindowedSinc("Windowed Sinc");
}

template<typename T> void resampler(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::ResampleImageFilter<InputImageType, InputImageType> ResampleFilterType;
	auto resampler = ResampleFilterType::New();
	typename ResampleFilterType::OriginPointType origin;
	origin[0] = parameters["Origin X"].toUInt(); origin[1] = parameters["Origin Y"].toUInt(); origin[2] = parameters["Origin Z"].toUInt();
	typename ResampleFilterType::SpacingType spacing;
	spacing[0] = parameters["Spacing X"].toDouble(); spacing[1] = parameters["Spacing Y"].toDouble(); spacing[2] = parameters["Spacing Z"].toDouble();
	typename ResampleFilterType::SizeType size;
	size[0] = parameters["Size X"].toUInt(); size[1] = parameters["Size Y"].toUInt(); size[2] = parameters["Size Z"].toUInt();
	QString interpolatorName = parameters["Interpolator"].toString();
	if (interpolatorName == InterpLinear)
	{
		typedef itk::LinearInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolatorName == InterpNearestNeighbour)
	{
		typedef itk::NearestNeighborInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolatorName == InterpBSpline)
	{
		typedef itk::BSplineInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolatorName == InterpWindowedSinc)
	{
		typedef itk::Function::HammingWindowFunction<3> WindowFunctionType;
		typedef itk::ZeroFluxNeumannBoundaryCondition<InputImageType> ConditionType;
		typedef itk::WindowedSincInterpolateImageFunction<
			InputImageType, 3,
			WindowFunctionType,
			ConditionType,
			double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	resampler->SetInput( dynamic_cast< InputImageType * >(filter->input()[0]->itkImage() ) );
	resampler->SetOutputOrigin( origin );
	resampler->SetOutputSpacing( spacing );
	resampler->SetSize( size );
	resampler->SetDefaultPixelValue( 0 );
	filter->progress()->observe( resampler );
	resampler->Update( );
	filter->addOutput( resampler->GetOutput() );
}

IAFILTER_CREATE(iAResampleFilter)

void iAResampleFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(resampler, inputPixelType(), this, parameters);
}

iAResampleFilter::iAResampleFilter() :
	iAFilter("Resample", "Geometric Transformations",
		"Resample the image to a new size.<br/>"
		"For more information, see the "
		"<a href=\"https ://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Filter</a> in the ITK documentation.")
{
	addParameter("Origin X", Discrete, 0);
	addParameter("Origin Y", Discrete, 0);
	addParameter("Origin Z", Discrete, 0);
	addParameter("Spacing X", Continuous, 0);
	addParameter("Spacing Y", Continuous, 0);
	addParameter("Spacing Z", Continuous, 0);
	addParameter("Size X", Discrete, 1, 1);
	addParameter("Size Y", Discrete, 1, 1);
	addParameter("Size Z", Discrete, 1, 1);
	QStringList interpolators;
	interpolators
		<< InterpLinear
		<< InterpNearestNeighbour
		<< InterpBSpline
		<< InterpWindowedSinc;
	addParameter("Interpolator", Categorical, interpolators);
}

QSharedPointer<iAFilterRunnerGUI> iAResampleFilterRunner::create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAResampleFilterRunner());
}

QMap<QString, QVariant> iAResampleFilterRunner::loadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = iAFilterRunnerGUI::loadParameters(filter, sourceMdi);
	params["Spacing X"] = sourceMdi->imagePointer()->GetSpacing()[0];
	params["Spacing Y"] = sourceMdi->imagePointer()->GetSpacing()[1];
	params["Spacing Z"] = sourceMdi->imagePointer()->GetSpacing()[2];
	params["Size X"] = sourceMdi->imagePointer()->GetDimensions()[0];
	params["Size Y"] = sourceMdi->imagePointer()->GetDimensions()[1];
	params["Size Z"] = sourceMdi->imagePointer()->GetDimensions()[2];
	return params;
}


template<typename T>
void extractImage(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::ExtractImageFilter< InputImageType, OutputImageType > EIFType;

	typename EIFType::InputImageRegionType::SizeType size;
	size[0] = parameters["Size X"].toUInt(); size[1] = parameters["Size Y"].toUInt(); size[2] = parameters["Size Z"].toUInt();
	typename EIFType::InputImageRegionType::IndexType index;
	index[0] = parameters["Index X"].toUInt(); index[1] = parameters["Index Y"].toUInt(); index[2] =	 parameters["Index Z"].toUInt();
	typename EIFType::InputImageRegionType region; region.SetIndex(index); region.SetSize(size);

	auto extractFilter = EIFType::New();
	extractFilter->SetInput(dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()));
	extractFilter->SetExtractionRegion(region);
	filter->progress()->observe(extractFilter);
	extractFilter->Update();

	filter->addOutput(setIndexOffsetToZero<T>(extractFilter->GetOutput()));
}

IAFILTER_CREATE(iAExtractImageFilter)

void iAExtractImageFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(extractImage, inputPixelType(), this, parameters);
}

iAExtractImageFilter::iAExtractImageFilter() :
	iAFilter("Extract Image", "Geometric Transformations",
		"Extract a part of the image.<br/>"
		"Both <em>Index</em> and <em>Size</em> values are in pixel units.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html\">"
		"Extract Image Filter</a> in the ITK documentation.")
{
	addParameter("Index X", Discrete, 0);
	addParameter("Index Y", Discrete, 0);
	addParameter("Index Z", Discrete, 0);
	addParameter("Size X", Discrete, 1, 1);
	addParameter("Size Y", Discrete, 1, 1);
	addParameter("Size Z", Discrete, 1, 1);
}

QSharedPointer<iAFilterRunnerGUI> iAExtractImageFilterRunner::create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAExtractImageFilterRunner());
}

QMap<QString, QVariant> iAExtractImageFilterRunner::loadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = iAFilterRunnerGUI::loadParameters(filter, sourceMdi);
	int const * dim = sourceMdi->imagePointer()->GetDimensions();
	if (params["Index X"].toUInt() >= static_cast<unsigned int>(dim[0]))
		params["Index X"] = 0;
	if (params["Index Y"].toUInt() >= static_cast<unsigned int>(dim[1]))
		params["Index Y"] = 0;
	if (params["Index Z"].toUInt() >= static_cast<unsigned int>(dim[2]))
		params["Index Z"] = 0;
	params["Size X"] = std::min(params["Size X"].toUInt(), dim[0] - params["Index X"].toUInt());
	params["Size Y"] = std::min(params["Size Y"].toUInt(), dim[1] - params["Index Y"].toUInt());
	params["Size Z"] = std::min(params["Size Z"].toUInt(), dim[2] - params["Index Z"].toUInt());
	return params;
}





template<typename T> void padImage(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::ConstantPadImageFilter<InputImageType, InputImageType> PadType;

	typename PadType::InputImageRegionType::SizeType lowerPadSize;
	lowerPadSize[0] = parameters["Lower X padding"].toUInt();
	lowerPadSize[1] = parameters["Lower Y padding"].toUInt();
	lowerPadSize[2] = parameters["Lower Z padding"].toUInt();
	typename PadType::InputImageRegionType::SizeType upperPadSize;
	upperPadSize[0] = parameters["Upper X padding"].toUInt();
	upperPadSize[1] = parameters["Upper Y padding"].toUInt();
	upperPadSize[2] = parameters["Upper Z padding"].toUInt();

	auto padFilter = PadType::New();
	padFilter->SetInput(dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()));
	padFilter->SetPadLowerBound(lowerPadSize);
	padFilter->SetPadUpperBound(upperPadSize);
	padFilter->SetConstant(parameters["Value"].toDouble());
	filter->progress()->observe(padFilter);
	padFilter->Update();

	filter->addOutput(setIndexOffsetToZero<T>(padFilter->GetOutput()));
}

IAFILTER_CREATE(iAPadImageFilter)

void iAPadImageFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(padImage, inputPixelType(), this, parameters);
}

iAPadImageFilter::iAPadImageFilter() :
	iAFilter("Pad Image", "Geometric Transformations",
		"Pad image on one or more side with given number of zero pixels.<br/>"
		"<em>Lower (x, y, z) padding</em> specifies the amount of pixels to be appended before the current first x/y/z pixel. "
		"<em>Upper (x, y, z) padding</em> specifies the amount of pixels to be appended after the current last x/y/z pixel. "
		"Pixels are added with the specified <em>Value</em><br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConstantPadImageFilter.html\">"
		"Extract Image Filter</a> in the ITK documentation.")
{
	addParameter("Lower X padding", Discrete, 1, 0);
	addParameter("Lower Y padding", Discrete, 1, 0);
	addParameter("Lower Z padding", Discrete, 1, 0);
	addParameter("Upper X padding", Discrete, 1, 0);
	addParameter("Upper Y padding", Discrete, 1, 0);
	addParameter("Upper Z padding", Discrete, 1, 0);
	addParameter("Value", Continuous, 0.0);
}

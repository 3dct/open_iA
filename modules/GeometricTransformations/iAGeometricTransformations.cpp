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
#include "iAGeometricTransformations.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include "mdichild.h"

#include <itkBSplineInterpolateImageFunction.h>
#include <itkChangeInformationImageFilter.h>
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

template<class T> void resampler_template(
	unsigned int originX, unsigned int originY, unsigned int originZ,
	double spacingX, double spacingY, double spacingZ,
	unsigned int sizeX, unsigned int sizeY, unsigned int sizeZ,
	QString const & interpolator,
	iAProgress* p, iAConnector* image  )
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::ResampleImageFilter< InputImageType, InputImageType >    ResampleFilterType;
	auto resampler = ResampleFilterType::New();

	typename ResampleFilterType::OriginPointType origin; origin[0] = originX; origin[1] = originY; origin[2] = originZ;
	typename ResampleFilterType::SpacingType spacing; spacing[0] = spacingX; spacing[1] = spacingY; spacing[2] = spacingZ;
	typename ResampleFilterType::SizeType size; size[0] = sizeX; size[1] = sizeY; size[2] = sizeZ;

	if (interpolator == InterpLinear)
	{
		typedef itk::LinearInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolator == InterpNearestNeighbour)
	{
		typedef itk::NearestNeighborInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolator == InterpBSpline)
	{
		typedef itk::BSplineInterpolateImageFunction<InputImageType, double> InterpolatorType;
		auto interpolator = InterpolatorType::New();
		resampler->SetInterpolator(interpolator);
	}
	else if (interpolator == InterpWindowedSinc)
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
	resampler->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	resampler->SetOutputOrigin( origin );
	resampler->SetOutputSpacing( spacing );
	resampler->SetSize( size );
	resampler->SetDefaultPixelValue( 0 );
	p->Observe( resampler );
	resampler->Update( );
	image->SetImage( resampler->GetOutput() );
	image->Modified();
	resampler->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAResampleFilter)

void iAResampleFilter::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(resampler_template, m_con->GetITKScalarPixelType(),
		parameters["Origin X"].toUInt(), parameters["Origin Y"].toUInt(), parameters["Origin Z"].toUInt(),
		parameters["Spacing X"].toDouble(), parameters["Spacing Y"].toDouble(), parameters["Spacing Z"].toDouble(),
		parameters["Size X"].toUInt(), parameters["Size Y"].toUInt(), parameters["Size Z"].toUInt(),
		parameters["Interpolator"].toString(),
		m_progress, m_con);
}

iAResampleFilter::iAResampleFilter() :
	iAFilter("Resample", "Geometric Transformations",
		"Resample the image to a new size.<br/>"
		"For more information, see the "
		"<a href=\"https ://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Filter</a> in the ITK documentation.")
{
	AddParameter("Origin X", Discrete, 0);
	AddParameter("Origin Y", Discrete, 0);
	AddParameter("Origin Z", Discrete, 0);
	AddParameter("Spacing X", Continuous, 0);
	AddParameter("Spacing Y", Continuous, 0);
	AddParameter("Spacing Z", Continuous, 0);
	AddParameter("Size X", Discrete, 1, 1);
	AddParameter("Size Y", Discrete, 1, 1);
	AddParameter("Size Z", Discrete, 1, 1);
	QStringList interpolators;
	interpolators
		<< InterpLinear
		<< InterpNearestNeighbour
		<< InterpBSpline
		<< InterpWindowedSinc;
	AddParameter("Interpolator", Categorical, interpolators);
}

QSharedPointer<iAFilterRunnerGUI> iAResampleFilterRunner::Create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAResampleFilterRunner());
}

QMap<QString, QVariant> iAResampleFilterRunner::LoadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = iAFilterRunnerGUI::LoadParameters(filter, sourceMdi);
	params["Spacing X"] = sourceMdi->getImagePointer()->GetSpacing()[0];
	params["Spacing Y"] = sourceMdi->getImagePointer()->GetSpacing()[1];
	params["Spacing Z"] = sourceMdi->getImagePointer()->GetSpacing()[2];
	params["Size X"] = sourceMdi->getImagePointer()->GetDimensions()[0];
	params["Size Y"] = sourceMdi->getImagePointer()->GetDimensions()[1];
	params["Size Z"] = sourceMdi->getImagePointer()->GetDimensions()[2];
	return params;
}


template<class T>
void extractImage_template(unsigned int indexX, unsigned int indexY, unsigned int indexZ,
	unsigned int sizeX, unsigned int sizeY, unsigned int sizeZ,
	iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::ExtractImageFilter< InputImageType, OutputImageType > EIFType;

	typename EIFType::InputImageRegionType::SizeType size; size[0] = sizeX; size[1] = sizeY; size[2] = sizeZ;
	typename EIFType::InputImageRegionType::IndexType index; index[0] = indexX; index[1] = indexY; index[2] = indexZ;
	typename EIFType::InputImageRegionType region; region.SetIndex(index); region.SetSize(size);

	auto filter = EIFType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetExtractionRegion(region);
	p->Observe(filter);
	filter->Update();

	//change the output image information - offset change to zero
	typename OutputImageType::IndexType idx; idx.Fill(0);
	typename OutputImageType::PointType origin; origin.Fill(0);
	typename OutputImageType::SizeType outsize; outsize[0] = sizeX;	outsize[1] = sizeY;	outsize[2] = sizeZ;
	typename OutputImageType::RegionType outreg;
	outreg.SetIndex(idx);
	outreg.SetSize(outsize);
	auto refimage = OutputImageType::New();
	refimage->SetRegions(outreg);
	refimage->SetOrigin(origin);
	refimage->SetSpacing(filter->GetOutput()->GetSpacing());
	refimage->Allocate();

	typedef itk::ChangeInformationImageFilter<OutputImageType> CIIFType;
	auto changefilter = CIIFType::New();
	changefilter->SetInput(filter->GetOutput());
	changefilter->UseReferenceImageOn();
	changefilter->SetReferenceImage(refimage);
	changefilter->SetChangeRegion(1);
	changefilter->Update();
	image->SetImage(changefilter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAExtractImageFilter)

void iAExtractImageFilter::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(extractImage_template, m_con->GetITKScalarPixelType(),
		parameters["Index X"].toUInt(), parameters["Index Y"].toUInt(), parameters["Index Z"].toUInt(),
		parameters["Size X"].toUInt(), parameters["Size Y"].toUInt(), parameters["Size Z"].toUInt(),
		m_progress, m_con);
}

iAExtractImageFilter::iAExtractImageFilter() :
	iAFilter("Extract Image", "Geometric Transformations",
		"Extract a part of the image.<br/>"
		"Both <em>Index</em> and <em>Size</em> values are in pixel units.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html\">"
		"Extract Image Filter</a> in the ITK documentation.")
{
	AddParameter("Index X", Discrete, 0);
	AddParameter("Index Y", Discrete, 0);
	AddParameter("Index Z", Discrete, 0);
	AddParameter("Size X", Discrete, 1, 1);
	AddParameter("Size Y", Discrete, 1, 1);
	AddParameter("Size Z", Discrete, 1, 1);
}

QSharedPointer<iAFilterRunnerGUI> iAExtractImageFilterRunner::Create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iAExtractImageFilterRunner());
}

QMap<QString, QVariant> iAExtractImageFilterRunner::LoadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = iAFilterRunnerGUI::LoadParameters(filter, sourceMdi);
	params["Size X"] = sourceMdi->getImagePointer()->GetDimensions()[0];
	params["Size Y"] = sourceMdi->getImagePointer()->GetDimensions()[1];
	params["Size Z"] = sourceMdi->getImagePointer()->GetDimensions()[2];
	return params;
}

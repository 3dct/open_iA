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
#include "iAIntensity.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkHistogramMatchingImageFilter.h>
#include <itkIntensityWindowingImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkNormalizeImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkTestingComparisonImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

template<class T>
int histomatch_template( int histogramLevels, int matchPoints, bool ThresholdAtMeanIntensity,  iAProgress* p, iAConnector* image2, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;

	typedef double InternalPixelType;
	typedef itk::Image< InternalPixelType, DIM > InternalImageType;
	typedef itk::CastImageFilter< ImageType, InternalImageType > FixedImageCasterType;
	typedef itk::CastImageFilter< ImageType, InternalImageType > MovingImageCasterType;
	typename FixedImageCasterType::Pointer fixedImageCaster = FixedImageCasterType::New();
	typename MovingImageCasterType::Pointer movingImageCaster = MovingImageCasterType::New();
	fixedImageCaster->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );
	movingImageCaster->SetInput( dynamic_cast< ImageType * >( image2->GetITKImage() ) );

	typedef itk::HistogramMatchingImageFilter<InternalImageType, InternalImageType > MatchingFilterType;
	MatchingFilterType::Pointer matcher = MatchingFilterType::New();
	matcher->SetInput( movingImageCaster->GetOutput() );
	matcher->SetReferenceImage( fixedImageCaster->GetOutput() );
	matcher->SetNumberOfHistogramLevels( histogramLevels );
	matcher->SetNumberOfMatchPoints( matchPoints );
	matcher->ThresholdAtMeanIntensityOn();

	p->Observe( matcher );
	matcher->Update();
	image->SetImage( matcher->GetOutput() );
	image->Modified();

	matcher->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T>
int normalize_template(iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::NormalizeImageFilter< ImageType, ImageType > NormalizeFilterType;
	typename NormalizeFilterType::Pointer normalizeFilter = NormalizeFilterType::New();
	normalizeFilter->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );
	normalizeFilter->Update( );

	p->Observe( normalizeFilter );
	normalizeFilter->Update();
	image->SetImage( normalizeFilter->GetOutput() );
	image->Modified();

	normalizeFilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T> 
int intensity_windowing_template( double wmin, double wmax, double omin, double omax, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::IntensityWindowingImageFilter <ImageType, ImageType> IntensityWindowingImageFilterType;
	typename IntensityWindowingImageFilterType::Pointer intensityWindowingFilter = IntensityWindowingImageFilterType::New();
	intensityWindowingFilter->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );
	intensityWindowingFilter->SetWindowMinimum( wmin );
	intensityWindowingFilter->SetWindowMaximum( wmax );
	intensityWindowingFilter->SetOutputMinimum( omin );
	intensityWindowingFilter->SetOutputMaximum( omax );
	intensityWindowingFilter->Update();

	p->Observe( intensityWindowingFilter );
	intensityWindowingFilter->Update();
	image->SetImage( intensityWindowingFilter->GetOutput() );
	image->Modified();

	intensityWindowingFilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T>
int rescaleImage_template(double outMin, double outMax, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;

	typedef itk::RescaleIntensityImageFilter< InputImageType, OutputImageType > RescalerType;
	typename RescalerType::Pointer filter = RescalerType::New();

	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));

	filter->SetOutputMinimum(outMin);
	filter->SetOutputMaximum(outMax);

	p->Observe(filter);
	filter->Update();

	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T> 
int mask_template( iAProgress* p, iAConnector* image2, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::MaskImageFilter< ImageType, ImageType > MaskFilterType;
	typename MaskFilterType::Pointer filter = MaskFilterType::New();
	filter->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );
	filter->SetMaskImage( dynamic_cast< ImageType * >( image2->GetITKImage() ) );

	p->Observe( filter );
	filter->Update();
	image->SetImage( filter->GetOutput() );
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T> 
int difference_template( double DifferenceThreshold, double ToleranceRadius, iAProgress* p, iAConnector* image2, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;

	typedef itk::Testing::ComparisonImageFilter<ImageType, ImageType> FilterType;


	typename FilterType::Pointer filter = FilterType::New();

	filter->SetDifferenceThreshold(DifferenceThreshold);
	filter->SetToleranceRadius(ToleranceRadius);

	filter->SetInput(dynamic_cast< ImageType * >( image->GetITKImage() ) );
	filter->SetInput(1,dynamic_cast< ImageType * >( image2->GetITKImage() ) );


	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T> int invert_intensity_template(  iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;

	typedef itk::InvertIntensityImageFilter< ImageType, ImageType> FilterType;
	typename FilterType::Pointer filter = FilterType::New();
	filter->SetInput(0, dynamic_cast< ImageType * >( image->GetITKImage() ) );

	p->Observe( filter );
	filter->Update(); 
	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iAIntensity::iAIntensity( QString fn, iAIntensityFilterType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent  )
	: iAAlgorithm( fn, i, p, logger, parent ), m_type(fid)
{}


void iAIntensity::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	switch (m_type)
	{
	case DIFFERENCE_IMAGE:
		getFixedConnector()->SetImage(image2); getFixedConnector()->Modified();
		ITK_TYPED_CALL(difference_template, itkType,
			DifferenceThreshold, ToleranceRadius, getItkProgress(), getFixedConnector(), getConnector());
		break;
	case INVERT_INTENSITY:
		ITK_TYPED_CALL(invert_intensity_template, itkType,
			getItkProgress(), getConnector());
		break;
	case MASK_IMAGE:
		getFixedConnector()->SetImage(image2); getFixedConnector()->Modified();
		ITK_TYPED_CALL(mask_template, itkType,
			getItkProgress(), getFixedConnector(), getConnector());
		break;
	case INTENSITY_WINDOWING:
		ITK_TYPED_CALL(intensity_windowing_template, itkType,
			windowMinimum, windowMaximum, outputMinimum, outputMaximum, getItkProgress(), getConnector());
		break;
	case NORMALIZE_IMAGE:
		ITK_TYPED_CALL(normalize_template, itkType, getItkProgress(), getConnector());
		break;
	case HISTOGRAM_MATCH:
		getFixedConnector()->SetImage(image2); getFixedConnector()->Modified();
		ITK_TYPED_CALL(histomatch_template, itkType,
			histogramLevels, matchPoints, thresholdAtMeanIntensity, getItkProgress(), getFixedConnector(), getConnector());
		break;
	case RESCALE_IMAGE:
		ITK_TYPED_CALL(rescaleImage_template, itkType,
			outputMin, outputMax, getItkProgress(), getConnector());
		break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}

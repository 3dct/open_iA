/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
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


void iAIntensity::run()
{
	switch (m_type)
	{
	case DIFFERENCE_IMAGE:
		difference(); break;
	case INVERT_INTENSITY:
		invert_intensity(); break;
	case MASK_IMAGE:
		mask(); break;
	case INTENSITY_WINDOWING:
		intensity_windowing(); break;
	case NORMALIZE_IMAGE:
		normalize(); break;
	case HISTOGRAM_MATCH:
		histomatch(); break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}

void iAIntensity::difference( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();
	getFixedConnector()->SetImage(image2); getFixedConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(difference_template, itkType,
			DifferenceThreshold, ToleranceRadius, getItkProgress(), getFixedConnector(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}

void iAIntensity::invert_intensity( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(invert_intensity_template, itkType,
			getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}

void iAIntensity::mask()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();
	getFixedConnector()->SetImage( image2 ); getFixedConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(mask_template, itkType,
			getItkProgress(), getFixedConnector(), getConnector());
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );

	emit startUpdate();
}

void iAIntensity::intensity_windowing()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(intensity_windowing_template, itkType,
			windowMinimum, windowMaximum, outputMinimum, outputMaximum, getItkProgress(), getConnector());
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );

	emit startUpdate();
}

void iAIntensity::normalize()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL( normalize_template, itkType, getItkProgress(), getConnector() );
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );

	emit startUpdate();
}

void iAIntensity::histomatch()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();
	getFixedConnector()->SetImage( image2 ); getFixedConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL( histomatch_template, itkType,
						histogramLevels, matchPoints, thresholdAtMeanIntensity, getItkProgress(), getFixedConnector(), getConnector() );
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );

	emit startUpdate();
}

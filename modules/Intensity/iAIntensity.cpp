/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAIntensity.h"
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkIntensityWindowingImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkMaskImageFilter.h>
#include <itkTestingComparisonImageFilter.h>

#include <QLocale>

/**
* template mask
*
* This template applies the intensity windowing image filter.
* \param	SetWindowMinimum	The window minimum.
* \param	SetWindowMaximum	The window maximum.
* \param	SetOutputMinimum	The output minimum.
* \param	SetOutputMaximum	The output maximum.
* \param	p					Filter progress information.
* \param	image				Input image.
* \param						The.
* \return	int Status-Code.
*/
template<class T> 
int intensity_windowing_template( double wmin, double wmax, double omin, double omax, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::IntensityWindowingImageFilter <ImageType, ImageType> IntensityWindowingImageFilterType;
	typename IntensityWindowingImageFilterType::Pointer filter = IntensityWindowingImageFilterType::New();
	filter->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );
	filter->SetWindowMinimum( wmin );
	filter->SetWindowMaximum( wmax );
	filter->SetOutputMinimum( omin );
	filter->SetOutputMaximum( omax );
	filter->Update();

	p->Observe( filter );
	filter->Update();
	image->SetImage( filter->GetOutput() );
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* template mask
*
* This template applies the mask image filter.
* \param	p					Filter progress information.
* \param 	image2		If non-null, the second image.
* \param	image				Input image.
* \param						The.
* \return	int Status-Code.
*/
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

/**
* template difference
* 
* This template applies the difference image filter. 
* \param	DifferenceThreshold	The difference threshold. 
* \param	ToleranceRadius		The tolerance radius. 
* \param	p					Filter progress information. 
* \param 	image2		If non-null, the second image. 
* \param	image				Input image. 
* \param						The. 
* \return	int Status-Code. 
*/
template<class T> 
int difference_template( double DifferenceThreshold, double ToleranceRadius, iAProgress* p, iAConnector* image2, iAConnector* image )
{
	typedef itk::Image< T, 3 > ImageType;

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

/**
* template invert_intensity
* 
* This template applies the invert intensity filter. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param			The. 
* \return	int Status-Code. 
*/
template<class T> int invert_intensity_template(  iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 > ImageType;

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

iAIntensity::iAIntensity( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent  )
	: iAFilter( fn, fid, i, p, logger, parent )
{
}

iAIntensity::~iAIntensity()
{
}

void iAIntensity::run()
{
	switch (getFilterID())
	{
	case DIFFERENCE_IMAGE:
		difference(); break;
	case INVERT_INTENSITY:
		invert_intensity(); break;
	case MASK_IMAGE:
		mask(); break;
	case INTENSITY_WINDOWING:
		intensity_windowing(); break;
	case UNKNOWN_FILTER: 
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

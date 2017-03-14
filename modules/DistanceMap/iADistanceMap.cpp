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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "pch.h"
#include "iADistanceMap.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkImageIOBase.h>
#include <itkDanielssonDistanceMapImageFilter.h>
#include <itkImage.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

template<class T> 
int signed_maurer_distancemap_template( int i, int s, int pos, int n, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;

	typedef itk::SignedMaurerDistanceMapImageFilter< InputImageType, RealImageType > SDDMType;
	typename SDDMType::Pointer distancefilter = SDDMType::New();

	distancefilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	distancefilter->SetBackgroundValue(0);

	if ( i == 2)
		distancefilter->UseImageSpacingOn();

	if ( s == 2 )		
		distancefilter->SquaredDistanceOff();

	if ( pos == 2 )
		distancefilter->InsideIsPositiveOn();

	p->Observe( distancefilter );

	distancefilter->Update(); 

	RealImageType::Pointer distanceImage = distancefilter->GetOutput();

	if ( n == 2 )
	{
		typedef itk::ImageRegionIterator<RealImageType> ImageIteratorType;
		ImageIteratorType iter ( distanceImage, distanceImage->GetLargestPossibleRegion() );
		iter.GoToBegin();
		while (!iter.IsAtEnd() )
		{
			if (iter.Get() < 0 )
				iter.Set(-1);			

			++iter;
		}
	}

	image->SetImage( distanceImage );
	image->Modified();

	distancefilter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T>
int danielsson_distancemap_template( iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< unsigned short, 3 >   UShortIageType;
	typedef itk::Image< unsigned char, 3 >   OutputImageType;

	typedef itk::DanielssonDistanceMapImageFilter< InputImageType, UShortIageType, UShortIageType > danielssonDistFilterType;
	typename danielssonDistFilterType::Pointer danielssonDistFilter = danielssonDistFilterType::New();
	danielssonDistFilter->InputIsBinaryOn();
	danielssonDistFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	p->Observe( danielssonDistFilter );
	danielssonDistFilter->Update();

	typedef itk::RescaleIntensityImageFilter< UShortIageType, OutputImageType > RescaleFilterType;
	typename RescaleFilterType::Pointer intensityRescaler = RescaleFilterType::New();
	intensityRescaler->SetInput( danielssonDistFilter->GetOutput() );
	intensityRescaler->SetOutputMinimum( 0 );
	intensityRescaler->SetOutputMaximum( 255 );
	intensityRescaler->Update();

	image->SetImage( intensityRescaler->GetOutput() );
	image->Modified();
	danielssonDistFilter->ReleaseDataFlagOn();
	intensityRescaler->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iADistanceMap::iADistanceMap( QString fn, iADistanceMapType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent ), m_type(fid)
{}

void iADistanceMap::run()
{
	switch (m_type)
	{
	case SIGNED_MAURER_DISTANCE_MAP: 
		signedmaurerdistancemap(); break;
	case DANIELSSON_DISTANCE_MAP:
		danielssondistancemap(); break;
	default:
		addMsg(tr("unknown filter type"));
	}
}

void iADistanceMap::signedmaurerdistancemap( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(signed_maurer_distancemap_template, itkType,
			imagespacing, squareddistance, insidepositive, n, getItkProgress(), getConnector());
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

void iADistanceMap::danielssondistancemap()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );

	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL( danielsson_distancemap_template, itkType, getItkProgress(), getConnector() );
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
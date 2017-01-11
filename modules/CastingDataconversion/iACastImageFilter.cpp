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
#include "iACastImageFilter.h"
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include <itkFHWRescaleIntensityImageFilter.h>

#include <vtkImageData.h>

#include <itkCastImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkLabelToRGBImageFilter.h>
#include <itkRGBPixel.h>
#include <itkRGBAPixel.h>

#include <QLocale>
#include <exception>

class myRGBATypeException : public std::exception
{
	virtual const char* what() const throw()
	{
		return "RGBA Conversion Error: LONG type needed.";
	}
} myRGBATypeExcep;

template<class T> int FHW_CastImage_template(std::string m_odt, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, DIM >   InputImageType;

	if ( m_odt.compare( "VTK_SIGNED_CHAR" ) == 0 )
	{
		typedef itk::Image <char, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_CHAR" ) == 0 )
	{
		typedef itk::Image <unsigned char, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_SHORT" ) == 0 )
	{
		typedef itk::Image <short, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_SHORT" ) == 0 )
	{
		typedef itk::Image <unsigned short, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_INT" ) == 0 )
	{
		typedef itk::Image <int, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_INT" ) == 0 )
	{
		typedef itk::Image <unsigned int, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_LONG" ) == 0 )
	{
		typedef itk::Image <long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_LONG" ) == 0 )
	{
		typedef itk::Image <unsigned long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_FLOAT" ) == 0 )
	{
		typedef itk::Image <float, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_DOUBLE" ) == 0 )
	{
		typedef itk::Image <double, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_LONG_LONG" ) == 0 )
	{
		typedef itk::Image <long long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_LONG_LONG" ) == 0 )
	{
		typedef itk::Image <unsigned long long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK__INT64" ) == 0 )
	{
		typedef itk::Image <long long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED__INT64" ) == 0 )
	{
		typedef itk::Image <unsigned long long, DIM> OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
	else if ( m_odt.compare( "Label image to color-coded RGBA image" ) == 0 )
	{
		if ( image->GetITKScalarPixelType() != itk::ImageIOBase::ULONG )
			throw  myRGBATypeExcep;
		
		typedef itk::Image< unsigned long, DIM > LongImageType;
		typedef itk::RGBPixel< unsigned char > RGBPixelType;
		typedef itk::Image< RGBPixelType, DIM > RGBImageType;
		typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
		typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;

		typedef itk::LabelToRGBImageFilter<LongImageType, RGBImageType> RGBFilterType;
		RGBFilterType::Pointer labelToRGBFilter = RGBFilterType::New();
		labelToRGBFilter->SetInput( dynamic_cast<LongImageType *>( image->GetITKImage() ) );
		labelToRGBFilter->Update();

		RGBImageType::RegionType region;
		region.SetSize( labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetSize() );
		region.SetIndex( labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetIndex() );

		RGBAImageType::Pointer rgbaImage = RGBAImageType::New();
		rgbaImage->SetRegions( region );
		rgbaImage->SetSpacing( labelToRGBFilter->GetOutput()->GetSpacing() );
		rgbaImage->Allocate();

		itk::ImageRegionConstIterator< RGBImageType > cit( labelToRGBFilter->GetOutput(), region );
		itk::ImageRegionIterator< RGBAImageType >     it( rgbaImage, region );
		for ( cit.GoToBegin(), it.GoToBegin(); !it.IsAtEnd(); ++cit, ++it )
		{
			it.Value().SetRed( cit.Value().GetRed() );
			it.Value().SetBlue( cit.Value().GetBlue() );
			it.Value().SetGreen( cit.Value().GetGreen() );
			it.Value().SetAlpha( 255 );
		}

		image->SetImage( rgbaImage );
		image->Modified();
		labelToRGBFilter->ReleaseDataFlagOn();
	}
	else
	{
		typedef itk::Image< T, DIM >   OutputImageType;
		typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
	return EXIT_SUCCESS;
}

iACastImageFilter::iACastImageFilter( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, fid, i, p, logger, parent )
{
}


void iACastImageFilter::run()
{
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );
	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();
	try
	{
		switch ( getFilterID() )
		{
			case FHW_CAST_IMAGE:
				fhwCastImage(); break;
			case DATATYPE_CONVERSION:
				DataTypeConversion(); break;
			default:
				addMsg( tr( "unknown filter type" ) );
		}
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	catch (const std::exception& e)
	{
		addMsg(tr("%1  %2 could not continue. %3").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(e.what()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(Stop()));
	emit startUpdate();
}

void iACastImageFilter::fhwCastImage()
{

	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL( FHW_CastImage_template, itkType,
					m_odt, getItkProgress(), getConnector() );
}

template<class T>
int DataTypeConversion_template(std::string m_odt, float m_min, float m_max, double m_outmin, double m_outmax, int dov, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;

	if ( m_odt.compare( "VTK_UNSIGNED_CHAR" ) == 0 )
	{
		typedef itk::Image <unsigned char, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( 0 );
			filter->SetOutputMaximum( 255 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_SHORT" ) == 0 )
	{
		typedef itk::Image <short, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( -32768 );
			filter->SetOutputMaximum( 32767 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_SHORT" ) == 0 )
	{
		typedef itk::Image <unsigned short, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( 0 );
			filter->SetOutputMaximum( 65535 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_INT" ) == 0 )
	{
		typedef itk::Image <int, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( -2147483648 );
			filter->SetOutputMaximum( 2147483647 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_UNSIGNED_INT" ) == 0 )
	{
		typedef itk::Image <unsigned int, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( 0 );
			filter->SetOutputMaximum( 4294967295 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_FLOAT" ) == 0 )
	{
		typedef itk::Image <float, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			double lower = 1.175*pow( 10.0, -38 );
			double higher = 3.402*pow( 10.0, 38 );
			filter->SetOutputMinimum( lower );
			filter->SetOutputMaximum( higher );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare( "VTK_DOUBLE" ) == 0 )
	{
		typedef itk::Image <double, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			double lower = 2.225*pow( 10.0, -307 );
			double higher = 1.797*pow( 10.0, 307 );
			filter->SetOutputMinimum( lower );
			filter->SetOutputMaximum( higher );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else
	{
		typedef itk::Image <char, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast<InputImageType *>( image->GetITKImage() ) );
		filter->SetInputMinimum( m_min );
		filter->SetInputMaximum( m_max );
		if ( dov == 2 )
		{
			filter->SetOutputMinimum( -128 );
			filter->SetOutputMaximum( 127 );
		}
		else
		{
			filter->SetOutputMinimum( m_outmin );
			filter->SetOutputMaximum( m_outmax );
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	return EXIT_SUCCESS;
}

void iACastImageFilter::DataTypeConversion()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL( DataTypeConversion_template, itkType,
					m_type, m_min, m_max, m_outmin, m_outmax, m_dov, getItkProgress(), getConnector() );
}

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

#include <vtkImageData.h>
#include <QLocale>

/**
* Fhw cast image template initializes itkCastImageFilter .
* \param	m_odt	Output image datatype.
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype.
* \return	int		Status code. 
*/
template<class T> int FHW_CastImage_template( string m_odt, iAProgress* p, iAConnector* image  )
{
	typedef itk::Image< T, 3 >   InputImageType;

	if ( m_odt.compare("VTK_SIGNED_CHAR") == 0 )
	{
		typedef itk::Image <char, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_CHAR") == 0 )
	{
		typedef itk::Image <unsigned char, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_SHORT")  == 0 )
	{
		typedef itk::Image <short, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_SHORT")  == 0 )
	{
		typedef itk::Image <unsigned short, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;

		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_INT")  == 0 )
	{
		typedef itk::Image <int, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_INT")  == 0 )
	{
		typedef itk::Image <unsigned int, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage( filter->GetOutput() );
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_LONG")  == 0 )
	{
		typedef itk::Image <long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_LONG")  == 0 )
	{
		typedef itk::Image <unsigned long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_FLOAT")  == 0 )
	{
		typedef itk::Image <float, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}	

	else if ( m_odt.compare("VTK_DOUBLE")  == 0 )
	{
		typedef itk::Image <double, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}	

	else if ( m_odt.compare("VTK_LONG_LONG")  == 0 )
	{
		typedef itk::Image <long long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_LONG_LONG")  == 0 )
	{
		typedef itk::Image <unsigned long long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK__INT64")  == 0 )
	{
		typedef itk::Image <long long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED__INT64")  == 0 )
	{
		typedef itk::Image <unsigned long long, 3> OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
	else
	{
		typedef itk::Image< T, 3 >   OutputImageType;
		typedef itk::CastImageFilter<InputImageType,OutputImageType> OTIFType;
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
	return EXIT_SUCCESS;
}

iACastImageFilter::iACastImageFilter( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAFilter( fn, fid, i, p, logger, parent )
{

}

iACastImageFilter::~iACastImageFilter()
{
}

void iACastImageFilter::run()
{
	switch (getFilterID())
	{
	case FHW_CAST_IMAGE: 
		fhwCastImage(); break;
	case DATATYPE_CONVERSION: 
		DataTypeConversion(); break;
	case UNKNOWN_FILTER: 
	default:
		addMsg(tr("unknown filter type"));
	}
}

void iACastImageFilter::fhwCastImage()
{

	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(FHW_CastImage_template, itkType,
			m_odt, getItkProgress(), getConnector());
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

/**
* Fhw cast image template initializes itkRescaleImagefilter .
* \param	m_odt	Output image datatype.
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype.
* \return	int		Status code. 
*/
template<class T> 
int DataTypeConversion_template( string m_odt, float m_min, float m_max, double m_outmin, double m_outmax, int dov, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;

	if ( m_odt.compare("VTK_UNSIGNED_CHAR") == 0 )
	{
		typedef itk::Image <unsigned char, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(0);
			filter->SetOutputMaximum(255);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_SHORT")  == 0 )
	{
		typedef itk::Image <short, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(-32768);
			filter->SetOutputMaximum(32767);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_SHORT")  == 0 )
	{
		typedef itk::Image <unsigned short, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(0);
			filter->SetOutputMaximum(65535);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_INT")  == 0 )
	{
		typedef itk::Image <int, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(-2147483648);
			filter->SetOutputMaximum(2147483647);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_UNSIGNED_INT")  == 0 )
	{
		typedef itk::Image <unsigned int, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(0);
			filter->SetOutputMaximum(4294967295);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	else if ( m_odt.compare("VTK_FLOAT")  == 0 )
	{
		typedef itk::Image <float, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			double lower = 1.175*pow(10.0,-38);
			double higher = 3.402*pow(10.0,38);
			filter->SetOutputMinimum(lower);
			filter->SetOutputMaximum(higher);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}	

	else if ( m_odt.compare("VTK_DOUBLE")  == 0 )
	{
		typedef itk::Image <double, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			double lower = 2.225*pow(10.0,-307);
			double higher = 1.797*pow(10.0,307);
			filter->SetOutputMinimum(lower);
			filter->SetOutputMaximum(higher);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}	

	else
	{
		typedef itk::Image <char, 3> OutputImageType;
		typedef itk::FHWRescaleIntensityImageFilter<InputImageType,OutputImageType> RIIFType;

		typename RIIFType::Pointer filter = RIIFType::New();
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		filter->SetInputMinimum(m_min);
		filter->SetInputMaximum(m_max);
		if ( dov == 2 )
		{
			filter->SetOutputMinimum(-128);
			filter->SetOutputMaximum(127);
		}
		else
		{
			filter->SetOutputMinimum(m_outmin);
			filter->SetOutputMaximum(m_outmax);
		}
		p->Observe( filter );
		filter->Update();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}

	return EXIT_SUCCESS;
}

/**
* Fhw cast image. 
*/

void iACastImageFilter::DataTypeConversion()
{

	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(DataTypeConversion_template, itkType,
			m_type, m_min, m_max, m_outmin, m_outmax, m_dov, getItkProgress(), getConnector());
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
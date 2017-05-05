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
#include "iABlurring.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>

#include <vtkImageData.h>

#include <QDateTime>
#include <QLocale>
#include <QString>

template<class T> 
int discrete_gaussian_template( double v, double me, int outimg, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;

	if ( outimg )
	{
		typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
		typename CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
		toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		typedef itk::DiscreteGaussianImageFilter< RealImageType, RealImageType > DGIFType;
		DGIFType::Pointer filter = DGIFType::New();
		filter->SetVariance( v );
		filter->SetMaximumError( me );
		filter->SetInput(  toReal->GetOutput() );
		filter->ReleaseDataFlagOn();

		typedef itk::CastImageFilter< RealImageType, InputImageType> CastToInputFilterType;
		typename CastToInputFilterType::Pointer toInput = CastToInputFilterType::New();
		toInput->SetInput( filter->GetOutput() );
		p->Observe( toInput );
		toInput->Update();

		image->SetImage( toInput->GetOutput() );
		image->Modified();
		toInput->ReleaseDataFlagOn();
	}
	else
	{
		typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
		typename CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();

		toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );

		typedef itk::DiscreteGaussianImageFilter< RealImageType, RealImageType > DGIFType;
		DGIFType::Pointer filter = DGIFType::New();

		filter->SetVariance( v );
		filter->SetMaximumError( me );
		filter->SetInput(  toReal->GetOutput() );
		p->Observe( filter );
		filter->Update(); 

		image->SetImage( filter->GetOutput() );
		image->Modified();

		filter->ReleaseDataFlagOn();
	}

	return EXIT_SUCCESS;
}


iABlurring::iABlurring( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{}


void iABlurring::run()
{
	discreteGaussian();
}


void iABlurring::discreteGaussian( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType pixelType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(discrete_gaussian_template, pixelType,
			variance, maximumError, outimg, getItkProgress(), getConnector() );
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

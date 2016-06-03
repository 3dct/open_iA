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
#include "iAThresholding.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkBinaryThresholdImageFilter.h>
#include <vtkImageData.h>
#include <QLocale>

/**
* template binary_threshold
* 
* This template applies a binary threshold filter. 
* \param	l		SetLowerThreshold. 
* \param	u		SetUpperThreshold. 
* \param	o		SetOutsideValue. 
* \param	i		SetInsideValue. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param			The. 
* \return	int Status-Code. 
*/
template<class T> int binary_threshold_template( double l, double u, double o, double i, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;

	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;
	typename BTIFType::Pointer filter = BTIFType::New();
	filter->SetLowerThreshold( T(l) );
	filter->SetUpperThreshold( T(u) );
	filter->SetOutsideValue( T(o) );
	filter->SetInsideValue( T(i) );
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );


	p->Observe( filter );

	filter->Update();

	image->SetImage( filter->GetOutput() );
	image->Modified();


	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iAThresholding::iAThresholding( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent )
	: iAFilter( fn, fid, i, p, logger, parent )
{
}

iAThresholding::~iAThresholding()
{
}

void iAThresholding::run()
{
	switch (getFilterID())
	{
	case BINARY_THRESHOLD: 
		binaryThresh(); break;
	case UNKNOWN_FILTER: 
	default:
		addMsg(tr("  unknown filter type"));
	}
}

void iAThresholding::binaryThresh(  )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(binary_threshold_template, itkType,
			lower, upper, outer, inner, getItkProgress(), getConnector());
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

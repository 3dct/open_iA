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
#include "iAMaximumDistance.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkMaximumDistance.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* template Maximum distance filter
* 
* This template applies a maximum distance algorithm
* 
* \param	parameter. 
* \param [in,out]	mdfHighInt_ptr	GetHighIntensity. 
* \param [in,out]	mdfLowInt_ptr	GetLowIntensity. 
* \param [in,out]	mdfThresh_ptr	GetThreshold. 
* \param	li						low intensity. 
* \param	b						SetBins. 
* \param	u						Centre set switch. 
* \param	p						Filter progress information. 
* \param	image					Input image. 
* \param							The. 
* \return	int Status-Code. 
*/
template<class T> 
int maximum_distance_template( int* mdfHighInt_ptr, int* mdfLowInt_ptr, int* mdfThresh_ptr, double li, double b, int u, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	// Define the thinning filter
	typedef itk::MaximumDistance< InputImageType >   MaximumDistanceType;
	typename MaximumDistanceType::Pointer filter = MaximumDistanceType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetBins(b);

	if ( u == 2 )
		filter->SetCentre(li);
	else 
		filter->SetCentre(32767);

	p->Observe( filter );

	filter->Update();

	image->SetImage( filter->GetOutput() );
	filter->GetThreshold(mdfThresh_ptr);
	filter->GetLowIntensity(mdfLowInt_ptr);
	filter->GetHighIntensity(mdfHighInt_ptr);

	image->Modified();
	return EXIT_SUCCESS;
}

iAMaximumDistance::iAMaximumDistance( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{}


void iAMaximumDistance::run()
{
	maximumDistance();
}

void iAMaximumDistance::maximumDistance()
{

	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(maximum_distance_template, itkType,
			&mdfHighInt, &mdfLowInt, &mdfThresh, li, b, u, getItkProgress(), getConnector());
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

	addMsg(tr("Maximum Distance Threshold = %1").arg(QString::number(mdfThresh, 10)) );
	addMsg(tr("Maximum Distance Low Peak = %1").arg(QString::number(mdfLowInt, 10)) );
	addMsg(tr("Maximum Distance High Peak = %1").arg(QString::number(mdfHighInt, 10)) );
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(Stop()));

	emit startUpdate();
}
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
#include "iAEdgeDetectionFilters.h"
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkImageIOBase.h>
#include <vtkImageData.h>
#include <QLocale>

/**
* Canny edge detection template initializes itkCannyEdgeDetectionImageFilter .
* \param	variance		The variance. 
* \param	maximumError	The maximum error. 
* \param	upper			The upper. 
* \param	lower			The lower. 
* \param	p				Filter progress information. 
* \param	image			Input image. 
* \param	T				Template datatype.
* \return	int				Status code 
*/
template<class T> 
int canny_edge_detection_template( double variance, double maximumError, double upper, double lower, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< float, 3 >   RealImageType;

	typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
	typename CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
	toReal->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );

	typedef itk::CannyEdgeDetectionImageFilter < RealImageType, RealImageType > CannyEDFType;
	CannyEDFType::Pointer canny = CannyEDFType::New(); 
	canny->SetVariance( variance );
	canny->SetMaximumError( maximumError );
	canny->SetUpperThreshold( upper );
	canny->SetLowerThreshold( lower );
	canny->SetInput( toReal->GetOutput() );

	p->Observe( canny );

	canny->Update();	
	image->SetImage(canny->GetOutput( ));
	image->Modified();

	canny->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

/**
* Constructor. 
* \param	fn		Filter name. 
* \param	fid		Filter ID number. 
* \param	i		Input image data. 
* \param	p		Input vtkpolydata. 
* \param	w		Input widget list. 
* \param	parent	Parent object. 
*/

iAEdgeDetectionFilters::iAEdgeDetectionFilters( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAFilter( fn, fid, i, p, logger, parent )
{

}

/**
* Destructor. 
*/

iAEdgeDetectionFilters::~iAEdgeDetectionFilters()
{
}

/**
* Execute the filter thread.
*/

void iAEdgeDetectionFilters::run()
{
	switch (getFilterID())
	{
	case CANNY_EDGE_DETECTION:
		cannyEdgeDetection(); break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}

/**
* Canny edge detection. 
*/

void iAEdgeDetectionFilters::cannyEdgeDetection()
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(canny_edge_detection_template, itkType,
			variance, maximumError, upper, lower, getItkProgress(), getConnector());
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


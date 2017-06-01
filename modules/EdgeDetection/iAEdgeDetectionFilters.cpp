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
#include "iAEdgeDetectionFilters.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCannyEdgeDetectionImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkImageIOBase.h>
#include <vtkImageData.h>

#include <QLocale>


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


iAEdgeDetectionFilters::iAEdgeDetectionFilters( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{}


void iAEdgeDetectionFilters::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(canny_edge_detection_template, itkType,
		variance, maximumError, upper, lower, getItkProgress(), getConnector());
}

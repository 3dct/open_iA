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
#include "iAGeneralThresholding.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkThresholdImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* General threshold template initializes itkThresholdImageFilter .
* \param	l		Lower threshold value. 
* \param	u		Upper threshold value. 
* \param	o		Value outside the range. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param	T		Template datatype. 
* \return	int		Status code. 
*/
template<class T> 
int general_threshold_template( double l, double u, double o, iAProgress* p, iAConnector* image )
{
	typedef itk::Image< T, 3 >   ImageType;
	typedef itk::ThresholdImageFilter <ImageType> GTIFType;
	typename GTIFType::Pointer filter = GTIFType::New();
	filter->SetLower( T(l) );
	filter->SetUpper( T(u) );
	filter->SetOutsideValue( T(o) );
	filter->ThresholdOutside(T(l), T(u));
	filter->SetInput( dynamic_cast< ImageType * >( image->GetITKImage() ) );

	p->Observe( filter );

	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iAGeneralThresholding::iAGeneralThresholding( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{}


void iAGeneralThresholding::performWork()
{
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(general_threshold_template, itkType,
		lower, upper, outer, getItkProgress(), getConnector());
}

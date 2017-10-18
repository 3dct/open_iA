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
#include "iASubtractImageFilter.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkImageIOBase.h>
#include <itkSubtractImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* template FHW_CastImage_template
* 
* This template applies a FHW CastImage filter. 
* \param	p				Filter progress information. 
* \param [in,out]	images	If non-null, the images. 
* \param					The. 
* \return	int Status-Code. 
*/
template<class T> 
int subtract_image_template( iAProgress* p, QVector<iAConnector*> images  )
{
	typedef itk::Image< T, 3 > InputImageType;
	typedef itk::Image< T, 3 > OutputImageType;
	typedef itk::SubtractImageFilter<InputImageType, InputImageType, OutputImageType> OTIFType;

	typename OTIFType::Pointer filter = OTIFType::New();

	filter->SetInput1( dynamic_cast< InputImageType * >( images[0]->GetITKImage() ) );
	filter->SetInput2( dynamic_cast< InputImageType * >( images[1]->GetITKImage() ) );

	p->Observe( filter );
	filter->Update();

	images[0]->SetImage( filter->GetOutput() );
	images[0]->Modified();

	filter->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iASubtractImageFilter::iASubtractImageFilter( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{}


void iASubtractImageFilter::performWork()
{
	AddImage(m_Image);
	iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
	ITK_TYPED_CALL(subtract_image_template, itkType,
		getItkProgress(), Connectors());
}

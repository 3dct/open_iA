/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAChanData.h"

#include "iAChannelVisualizationData.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkTransform.h>

iAChanData::iAChanData( const QList<QColor> & colors, iAChannelID chanId ) :
	visData( new iAChannelVisualizationData ),
	imgData( vtkSmartPointer<vtkImageData>::New() ),
	tf( vtkSmartPointer<vtkColorTransferFunction>::New() ),
	otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	vol_otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	cols( colors ),
	id( chanId ),
	scalarBarWgt( vtkSmartPointer<vtkScalarBarWidget>::New() )
{}

iAChanData::iAChanData( QColor c1, QColor c2, iAChannelID chanId ) :
	visData( new iAChannelVisualizationData ),
	imgData( vtkSmartPointer<vtkImageData>::New() ),
	tf( vtkSmartPointer<vtkColorTransferFunction>::New() ),
	otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	vol_otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	id( chanId ),
	scalarBarWgt( vtkSmartPointer<vtkScalarBarWidget>::New() )
{
	cols.push_back( c1 );
	cols.push_back( c2 );
}

void iAChanData::InitTFs()
{
	tf->RemoveAllPoints();
	double rangeVal = imgData->GetScalarRange()[0];
	double rangeDelta = (imgData->GetScalarRange()[1] - rangeVal) / (cols.size() - 1);
	for( int i = 0; i < cols.size(); ++i )
	{
		tf->AddRGBPoint( rangeVal, cols[i].redF(), cols[i].greenF(), cols[i].blueF() );
		rangeVal += rangeDelta;
	}
	tf->Build();
	scalarBarWgt->GetScalarBarActor()->SetLookupTable( tf );
	//	BuildDefaultTF( m_chImgData, m_chTF, chCol );
	otf->AddPoint( imgData->GetScalarRange()[0], 0.0 );
	otf->AddPoint( 0.0001, 1.0 );
	otf->AddPoint( imgData->GetScalarRange()[1], 1.0 );
	vol_otf->AddPoint( imgData->GetScalarRange()[0],			0.0 );
	vol_otf->AddPoint( imgData->GetScalarRange()[1] - 0.0002,	0.05 );
	vol_otf->AddPoint( imgData->GetScalarRange()[1] - 0.0001,	1.0 );
	vol_otf->AddPoint( imgData->GetScalarRange()[1],			1.0 );
	visData->SetColor( cols[0] );
	ResetChannel( visData.data(), imgData, tf, otf );
}
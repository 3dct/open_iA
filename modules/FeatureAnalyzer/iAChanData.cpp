// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChanData.h"

#include "iAChannelData.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>

iAChanData::iAChanData( const QList<QColor> & colors, uint chanId ) :
	visData( new iAChannelData ),
	imgData( vtkSmartPointer<vtkImageData>::New() ),
	tf( vtkSmartPointer<vtkColorTransferFunction>::New() ),
	otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	vol_otf( vtkSmartPointer<vtkPiecewiseFunction>::New() ),
	cols( colors ),
	id( chanId ),
	scalarBarWgt( vtkSmartPointer<vtkScalarBarWidget>::New() )
{}

iAChanData::iAChanData( QColor c1, QColor c2, uint chanId ) :
	visData( new iAChannelData ),
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
	visData->setData(imgData, tf, otf );
}

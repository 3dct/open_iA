// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABoundingBoxVisModule.h"

#include "iA4DCTVisWin.h"

#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iABoundingBoxVisModule::iABoundingBoxVisModule( )
	: iAVisModule( )
{
	m_cubeSource = vtkSmartPointer<vtkCubeSource>::New();

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputConnection( m_cubeSource->GetOutputPort( ) );

	m_actor = vtkSmartPointer<vtkActor>::New();
	m_actor->SetMapper( m_mapper );
	m_actor->GetProperty( )->SetRepresentationToWireframe( );
	m_actor->GetProperty( )->LightingOff( );
}

void iABoundingBoxVisModule::show( )
{
	m_renderer->AddActor( m_actor );
}

void iABoundingBoxVisModule::hide( )
{
	m_renderer->RemoveActor( m_actor );
}

void iABoundingBoxVisModule::setSize( double * size )
{
	m_cubeSource->SetBounds( 0, size[0] * SCENE_SCALE, 0, size[1] * SCENE_SCALE, 0, size[2] * SCENE_SCALE );
}

void iABoundingBoxVisModule::setColor( double r, double g, double b )
{
	// set settings
	settings.Color = QColor( r * 255, g * 255, b * 255 );
	// update visualization
	m_actor->GetProperty( )->SetColor( r, g, b );
}

void iABoundingBoxVisModule::setPosition( double x, double y, double z )
{
	m_actor->SetPosition( x, y, z );
}

void iABoundingBoxVisModule::setLineWidth( float w )
{
	m_actor->GetProperty( )->SetLineWidth( w );
}

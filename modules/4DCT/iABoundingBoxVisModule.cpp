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
#include "iABoundingBoxVisModule.h"
// iA
#include "iA4DCTVisWin.h"
// vtk
#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iABoundingBoxVisModule::iABoundingBoxVisModule( )
	: iAVisModule( )
{
	m_cubeSource = vtkSmartPointer<vtkCubeSource>::New( );

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_mapper->SetInputConnection( m_cubeSource->GetOutputPort( ) );

	m_actor = vtkSmartPointer<vtkActor>::New( );
	m_actor->SetMapper( m_mapper );
	m_actor->GetProperty( )->SetRepresentationToWireframe( );
	m_actor->GetProperty( )->LightingOff( );
}

void iABoundingBoxVisModule::enable( )
{
	if( !isAttached( ) ) return;
	if( !isEnabled( ) ) {
		m_renderer->AddActor( m_actor );
	}
	iAVisModule::enable( );
}

void iABoundingBoxVisModule::disable( )
{
	if( !isAttached( ) ) return;
	if( isEnabled( ) ) {
		m_renderer->RemoveActor( m_actor );
	}
	iAVisModule::disable( );
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
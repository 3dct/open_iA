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
#include "iADefectVisModule.h"
// iA
#include "iA4DCTVisWin.h"
// vtk
#include <vtkRenderer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkOBJReader.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkProperty.h>

iADefectVisModule::iADefectVisModule( )
	: iAVisModule( )
{
	m_reader = vtkSmartPointer<vtkOBJReader>::New( );

	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New( );
	transform->Scale( SCENE_SCALE, SCENE_SCALE, SCENE_SCALE );

	vtkSmartPointer<vtkTransformFilter> transformFilter = vtkSmartPointer<vtkTransformFilter>::New( );
	transformFilter->SetInputConnection( m_reader->GetOutputPort( ) );
	transformFilter->SetTransform( transform );

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_mapper->SetInputConnection( transformFilter->GetOutputPort( ) );
	m_actor = vtkSmartPointer<vtkActor>::New( );
	m_actor->SetMapper( m_mapper );
}

void iADefectVisModule::show( )
{
	m_renderer->AddActor( m_actor );
}

void iADefectVisModule::hide( )
{
	m_renderer->RemoveActor( m_actor );
}

void iADefectVisModule::setInputFile( QString path )
{
	m_reader->SetFileName( path.toStdString( ).c_str( ) );
	m_reader->Update( );
}

void iADefectVisModule::setColor( double r, double g, double b )
{
	m_actor->GetProperty( )->SetColor( r, g, b );
}

void iADefectVisModule::setOpacity( double opacity )
{
	m_actor->GetProperty( )->SetOpacity( opacity );
}

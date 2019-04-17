/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iADefectVisModule.h"

#include "iA4DCTVisWin.h"

#include <io/iAFileUtils.h>

#include <vtkActor.h>
#include <vtkOBJReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

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
	m_reader->SetFileName( getLocalEncodingFileName(path).c_str( ) );
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

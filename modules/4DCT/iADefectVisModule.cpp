// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefectVisModule.h"

#include "iA4DCTVisWin.h"

#include <iAFileUtils.h>

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
	m_reader = vtkSmartPointer<vtkOBJReader>::New();

	auto transform = vtkSmartPointer<vtkTransform>::New();
	transform->Scale( SCENE_SCALE, SCENE_SCALE, SCENE_SCALE );

	auto transformFilter = vtkSmartPointer<vtkTransformFilter>::New();
	transformFilter->SetInputConnection( m_reader->GetOutputPort( ) );
	transformFilter->SetTransform( transform );

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputConnection( transformFilter->GetOutputPort( ) );
	m_actor = vtkSmartPointer<vtkActor>::New();
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

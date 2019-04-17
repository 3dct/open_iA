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
#include "iAPlaneVisModule.h"

#include "iA4DCTVisWin.h"

#include <vtkImageCast.h>
#include <vtkImageShiftScale.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTexture.h>

iAPlaneVisModule::iAPlaneVisModule( )
	: iAVisModule( )
	//, m_dir(Direction::XY)
{
	m_plane = vtkSmartPointer<vtkPlaneSource>::New( );
	m_plane->SetResolution( 5, 5 );
	m_texture = vtkSmartPointer<vtkTexture>::New( );
	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_mapper->SetInputConnection( m_plane->GetOutputPort( ) );
	m_actor = vtkSmartPointer<vtkActor>::New( );
	m_actor->SetMapper( m_mapper );
	m_actor->SetTexture( m_texture );
	m_actor->GetProperty( )->LightingOff( );

	// default settings
	setDirXY( );
	setOpacity( 1. );
	setSlice( 0. );
	disableShading( );
}

void iAPlaneVisModule::show( )
{
	m_renderer->AddActor( m_actor );
}

void iAPlaneVisModule::hide( )
{
	m_renderer->RemoveActor( m_actor );
}

void iAPlaneVisModule::setSize( double * size )
{
	m_size[0] = size[0]; m_size[1] = size[1]; m_size[2] = size[2];
	setPlanePosition( 0 );
}

void iAPlaneVisModule::setImage( iA4DCTFileData fileName )
{
	double scale = (double)0xff / 0xffff;
	vtkSmartPointer<vtkImageShiftScale> shifter = vtkSmartPointer<vtkImageShiftScale>::New( );
	shifter->SetShift( 0. );
	shifter->SetScale( scale );
	shifter->SetOutputScalarTypeToUnsignedChar( );
	shifter->SetInputConnection( iA4DCTFileManager::getInstance( ).getOutputPort( fileName ) );
	shifter->ReleaseDataFlagOff( );
	shifter->Update( );

	// set image
	m_img = shifter->GetOutput( );

	int extent[6];
	m_img->GetExtent( extent );
	m_imgSize[0] = extent[1] - extent[0];
	m_imgSize[1] = extent[3] - extent[2];
	m_imgSize[2] = extent[5] - extent[4];
	m_img->GetSpacing( m_imgSpacing );

	m_reslice = vtkSmartPointer<vtkImageReslice>::New( );
	m_reslice->SetInputData( m_img );

	setSlice( 0 );
}

void iAPlaneVisModule::setSlice( int slice )
{
	// update visualization
	if( m_reslice.GetPointer( ) == nullptr )
		return;
	static double axialElementsXY[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	static double axialElementsXZ[16] = {
		1, 0, 0, 0,
		0, 0, 1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1 };

	static double axialElementsYZ[16] = {
		0, 0, 1, 0,
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1 };

	vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New( );

	double sliceNum;
	switch( settings.Dir ) {
	case iAPlaneVisSettings::Direction::XY:
		settings.Slice[0] = slice;
		sliceNum = slice * m_imgSpacing[2];
		resliceAxes->DeepCopy( axialElementsXY );
		break;
	case iAPlaneVisSettings::Direction::XZ:
		settings.Slice[1] = slice;
		sliceNum = slice * m_imgSpacing[1];
		resliceAxes->DeepCopy( axialElementsXZ );
		break;
	case iAPlaneVisSettings::Direction::YZ:
		settings.Slice[2] = slice;
		sliceNum = slice * m_imgSpacing[0];
		resliceAxes->DeepCopy( axialElementsYZ );
		break;
	}

	resliceAxes->SetElement( 0, 3, sliceNum );
	resliceAxes->SetElement( 1, 3, sliceNum );
	resliceAxes->SetElement( 2, 3, sliceNum );

	m_reslice->SetOutputDimensionality( 2 );
	m_reslice->SetResliceAxes( resliceAxes );
	m_reslice->Update( );

	m_texture->SetInputConnection( m_reslice->GetOutputPort( ) );

	setPlanePosition( slice );
}

void iAPlaneVisModule::setOpacity( double opacity )
{
	// set settings
	settings.Opacity = opacity;
	// update visualization
	m_actor->GetProperty( )->SetOpacity( opacity );
}

void iAPlaneVisModule::enableShading( )
{
	// set settings
	settings.Shading = true;
	// update visualization
	m_actor->GetProperty( )->LightingOn( );
}

void iAPlaneVisModule::disableShading( )
{
	// set settings
	settings.Shading = false;
	// update visualization
	m_actor->GetProperty( )->LightingOff( );
}

void iAPlaneVisModule::setDirXY( )
{
	settings.Dir = iAPlaneVisSettings::Direction::XY;
	setSlice( settings.Slice[0] );
}

void iAPlaneVisModule::setDirXZ( )
{
	settings.Dir = iAPlaneVisSettings::Direction::XZ;
	setSlice( settings.Slice[1] );
}

void iAPlaneVisModule::setDirYZ( )
{
	settings.Dir = iAPlaneVisSettings::Direction::YZ;
	setSlice( settings.Slice[2] );
}

void iAPlaneVisModule::setPlanePosition( int slice )
{
	switch( settings.Dir )
	{
	case iAPlaneVisSettings::Direction::XY:
	{
		double zPos = (double)slice / m_imgSize[2] * m_size[2];
		m_plane->SetOrigin( 0, 0, zPos * SCENE_SCALE );
		m_plane->SetPoint1( m_size[0] * SCENE_SCALE, 0, zPos * SCENE_SCALE );
		m_plane->SetPoint2( 0, m_size[1] * SCENE_SCALE, zPos * SCENE_SCALE );
		break;
	}
	case iAPlaneVisSettings::Direction::XZ:
	{
		double yPos = (double)slice / m_imgSize[1] * m_size[1];
		m_plane->SetOrigin( 0, yPos * SCENE_SCALE, 0 );
		m_plane->SetPoint1( m_size[0] * SCENE_SCALE, yPos * SCENE_SCALE, 0 );
		m_plane->SetPoint2( 0, yPos * SCENE_SCALE, m_size[2] * SCENE_SCALE );
		break;
	}
	case iAPlaneVisSettings::Direction::YZ:
	{
		double xPos = (double)slice / m_imgSize[0] * m_size[0];
		m_plane->SetOrigin( xPos * SCENE_SCALE, 0, 0 );
		m_plane->SetPoint1( xPos * SCENE_SCALE, m_size[1] * SCENE_SCALE, 0 );
		m_plane->SetPoint2( xPos * SCENE_SCALE, 0, m_size[2] * SCENE_SCALE );
		break;
	}
	}
}

void iAPlaneVisModule::getImageSize( int * imgSize )
{
	imgSize[0] = m_imgSize[0]; imgSize[1] = m_imgSize[1]; imgSize[2] = m_imgSize[2];
}

void iAPlaneVisModule::enableHighlighting( bool enable )
{
	if( enable )
		m_reslice->SetInputData( m_colorImg );
	else
		m_reslice->SetInputData( m_img );
	m_plane->Modified( );
}
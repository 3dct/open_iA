// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTRegionMarkerModule.h"

#include <vtkActor.h>
#include <vtkCellPicker.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// Define interaction style
class MouseInteractorStylePP : public vtkInteractorStyleTrackballCamera
{
public:
	static MouseInteractorStylePP* New( );
	vtkTypeMacro( MouseInteractorStylePP, vtkInteractorStyleTrackballCamera );

	void OnLeftButtonDown( ) override
	{
		if( m_lCntrlIsPressed && m_rm )
		{
			vtkSmartPointer<vtkCellPicker> pointPicker = vtkSmartPointer<vtkCellPicker>::New( );
			int pos[2]; this->Interactor->GetEventPosition( pos );
			if( pointPicker->Pick( pos[0], pos[1], 0, this->Interactor->GetRenderWindow( )->GetRenderers( )->GetFirstRenderer( ) ) )
			{
				double picked[3];
				pointPicker->GetPickPosition( picked );
				m_rm->addRegion( picked );
			}
		}
		// forward the event
		vtkInteractorStyleTrackballCamera::OnLeftButtonDown( );
	}

	void OnKeyDown( ) override
	{
		vtkRenderWindowInteractor *rwi = this->Interactor;
		std::string key = rwi->GetKeySym( );
		if( key == "Control_L" )
		{
			m_lCntrlIsPressed = true;
		}
		// Forward events
		vtkInteractorStyleTrackballCamera::OnKeyDown( );
	}

	void OnKeyUp( ) override
	{
		vtkRenderWindowInteractor *rwi = this->Interactor;
		std::string key = rwi->GetKeySym( );
		if( key == "Control_L" )
		{
			m_lCntrlIsPressed = false;
		}
		// Forward events
		vtkInteractorStyleTrackballCamera::OnKeyUp( );
	}

	void SetRegionMarker( iA4DCTRegionMarkerModule* rm ) { m_rm = rm; }

private:
	iA4DCTRegionMarkerModule* m_rm = nullptr;
	bool	m_lCntrlIsPressed = false;
};

vtkStandardNewMacro( MouseInteractorStylePP );


iA4DCTRegionMarkerModule::iA4DCTRegionMarkerModule( )
	: iAVisModule( )
{ }

void iA4DCTRegionMarkerModule::show( )
{
	vtkSmartPointer<vtkPointPicker> pointPicker = vtkSmartPointer<vtkPointPicker>::New( );
	m_renderer->GetRenderWindow( )->GetInteractor( )->SetPicker( pointPicker );
	vtkSmartPointer<MouseInteractorStylePP> style = vtkSmartPointer<MouseInteractorStylePP>::New( );
	style->SetRegionMarker( this );
	m_renderer->GetRenderWindow( )->GetInteractor( )->SetInteractorStyle( style );
}

void iA4DCTRegionMarkerModule::hide( )
{ }

void iA4DCTRegionMarkerModule::addRegion( double* pos )
{
	vtkSmartPointer<vtkSphereSource> sphereSrc = vtkSmartPointer<vtkSphereSource>::New( );
	sphereSrc->SetRadius( 3 );
	vtkSmartPointer<vtkPolyDataMapper> m_sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New( );
	m_sphereMapper->SetInputConnection( sphereSrc->GetOutputPort( ) );

	vtkSmartPointer<vtkActor> sphere = vtkSmartPointer<vtkActor>::New( );
	sphere->SetMapper( m_sphereMapper );
	sphere->GetProperty( )->SetColor( 1, 0, 0 );
	sphere->GetProperty( )->SetAmbient( 0.3 );
	sphere->GetProperty( )->SetDiffuse( 0.0 );
	sphere->GetProperty( )->SetSpecular( 1.0 );
	sphere->GetProperty( )->SetSpecularPower( 5.0 );
	sphere->GetProperty( )->SetOpacity( 0.5 );
	sphere->AddPosition( pos );
	m_renderer->AddActor( sphere );
}

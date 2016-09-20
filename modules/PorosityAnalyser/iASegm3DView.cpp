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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iASegm3DView.h"

#include "defines.h"
#include "iAFast3DMagicLensWidget.h"
#include "iARenderer.h"
#include "iARendererManager.h"
#include "iAPerceptuallyUniformLUT.h"
#include "iARenderSettings.h"
#include "iATransferFunction.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeSettings.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>

#include <QSettings>
#include <QHBoxLayout>
#include <QLabel>

iASegm3DView::iASegm3DView( QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : Segm3DViewContainer( parent, f ),
	m_layout( new QHBoxLayout( wgtContainer ) ),
	m_renMgr( new iARendererManager ),
	m_range( 0.0 )
{}

void iASegm3DView::SetDataToVisualize( QList<vtkImageData*> imgData, QList<vtkPolyData*> polyData, QList<vtkPiecewiseFunction*> otf, QList<vtkColorTransferFunction*> ctf, QStringList slicerNames )
{
	m_range = 0.0;
	m_renMgr->removeAll();
	foreach( iASegm3DViewData* sd, m_data )
	{
		m_layout->removeWidget( sd->GetWidget() );
		delete sd;
	}
	m_data.clear();

	foreach( QWidget* c, m_containerList )
	{
		m_layout->removeWidget( c );
		delete c;
	}
	m_containerList.clear();

	int sz = imgData.size();
	for( int i = 0; i < sz; ++i )
	{
		QWidget * container = new QWidget( wgtContainer );
		QVBoxLayout *  containerLayout = new QVBoxLayout( container );
		container->setLayout( containerLayout );
		m_containerList.append( container );

		QLabel * selTextLabel = new QLabel( slicerNames[i], container );
		selTextLabel->setAlignment( Qt::AlignCenter );
		selTextLabel->setFixedHeight( 15 );
		selTextLabel->setStyleSheet( "font-weight: bold;" );
		containerLayout->addWidget( selTextLabel );

		iASegm3DViewData * sd = new iASegm3DViewData( &m_range, this );
		containerLayout->addWidget( sd->GetWidget() );
		sd->GetWidget()->setParent( container );
		sd->SetDataToVisualize( imgData[i], polyData[i], otf[i], ctf[i] );
		m_data.push_back( sd );
		m_renMgr->addToBundle( sd->GetRenderer() );

		m_layout->addWidget( container );
	}
	foreach( iASegm3DViewData* sd, m_data )
		sd->UpdateRange();
}

void iASegm3DView::SetPolyData( QList<vtkPolyData*> polyData )
{
	m_range = 0.0;
	int sz = polyData.size();
	for( int i = 0; i < sz; ++i )
	{
		iASegm3DViewData * sd = m_data[i];
		sd->SetPolyData( polyData[i] );
	}
	foreach( iASegm3DViewData* sd, m_data )
		sd->UpdateRange();
}

iASegm3DView::~iASegm3DView()
{
	foreach( iASegm3DViewData* sd, m_data )
		delete sd;
}

void iASegm3DView::ShowVolume( bool visible )
{
	foreach( iASegm3DViewData* sd, m_data )
		sd->ShowVolume( visible );
}

void iASegm3DView::ShowSurface( bool visible )
{
	foreach( iASegm3DViewData* sd, m_data )
		sd->ShowSurface( visible );
}

void iASegm3DView::SetSensitivity( double sensitivity )
{
	m_range = 0.0;
	foreach( iASegm3DViewData* sd, m_data )
		sd->SetSensitivity( sensitivity );
	foreach( iASegm3DViewData* sd, m_data )
		sd->UpdateRange();
}

void iASegm3DView::ShowWireframe( bool visible )
{
	foreach( iASegm3DViewData* sd, m_data )
		sd->ShowWireframe( visible );
}


iASegm3DViewData::iASegm3DViewData( double * rangeExt, QWidget * parent ) :
	m_renderer( new iARenderer( parent ) ),
	m_rendInitialized( false ),
	m_axesTransform( vtkSmartPointer<vtkTransform>::New() ),
	m_observedRenderer( 0 ),
	m_tag( 0 ),
	m_wgt( new iAFast3DMagicLensWidget ),
	scalarBarWgt( vtkSmartPointer<vtkScalarBarWidget>::New() ),
	volScalarBarWgt( vtkSmartPointer<vtkScalarBarWidget>::New() ),
	m_sensitivity( 1.0 ),
	m_wireMapper( vtkSmartPointer<vtkPolyDataMapper>::New() ),
	m_wireActor( vtkSmartPointer<vtkActor>::New() ),
	m_rangeExt( rangeExt ),
	m_lut( vtkSmartPointer<vtkLookupTable>::New() )
{
	scalarBarWgt->SetRepositionable( true );
	scalarBarWgt->SetResizable( true );
	scalarBarWgt->GetScalarBarRepresentation()->SetOrientation( 1 );
	scalarBarWgt->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue( 0.92, 0.2 );
	scalarBarWgt->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue( 0.06, 0.75 );
	vtkScalarBarActor *scalarBarActor = scalarBarWgt->GetScalarBarActor();
	scalarBarActor->SetTitle( "Distance" );
	scalarBarActor->SetNumberOfLabels( 4 );
	vtkPolyDataMapper * mapper = m_renderer->GetPolyMapper();
	double sr[2];  mapper->GetScalarRange(sr);
	iAPerceptuallyUniformLUT::BuildPerceptuallyUniformLUT( m_lut, sr, 256 );
	m_lut->SetRange( sr ); m_lut->SetTableRange( sr );
	mapper->SetLookupTable( m_lut );
	scalarBarActor->SetLookupTable( m_lut );

	volScalarBarWgt->SetRepositionable( true );
	volScalarBarWgt->SetResizable( true );
	volScalarBarWgt->GetScalarBarRepresentation()->SetOrientation( 1 );
	volScalarBarWgt->GetScalarBarRepresentation()->GetPositionCoordinate()->SetValue( 0.02, 0.2 );
	volScalarBarWgt->GetScalarBarRepresentation()->GetPosition2Coordinate()->SetValue( 0.06, 0.75 );
	volScalarBarWgt->GetScalarBarActor()->SetTitle( "Mask count" );
	volScalarBarWgt->GetScalarBarActor()->SetNumberOfLabels( 4 );

	m_wireMapper->SetResolveCoincidentTopologyPolygonOffsetParameters( 1, 1 );
	m_wireMapper->SetResolveCoincidentTopologyToPolygonOffset();
	m_wireMapper->ScalarVisibilityOff();
	m_wireActor->SetMapper( m_wireMapper );
	m_wireActor->GetProperty()->SetColor( 0.0, 0.0, 0.0 );
	m_wireActor->GetProperty()->SetRepresentationToWireframe();
	m_wireActor->GetProperty()->SetAmbient( 1.0 );
	m_wireActor->GetProperty()->SetDiffuse( 0.0 );
	m_wireActor->GetProperty()->SetSpecular( 0.0 );
	m_renderer->GetRenderer()->AddActor( m_wireActor );

	m_wgt->SetRenderWindow( (vtkGenericOpenGLRenderWindow* )m_renderer->GetRenderWindow() );
	m_renderer->setAxesTransform( m_axesTransform );

	QObject::connect( m_wgt, SIGNAL( rightButtonReleasedSignal() ), m_renderer, SLOT( mouseRightButtonReleasedSlot() ) );
	QObject::connect( m_wgt, SIGNAL( leftButtonReleasedSignal() ), m_renderer, SLOT( mouseLeftButtonReleasedSlot() ) );
}

iASegm3DViewData::~iASegm3DViewData()
{
	delete m_renderer;
	delete m_wgt;
}

void iASegm3DViewData::removeObserver()
{
	//is m_renderer deleted by Qt?
	if( m_observedRenderer )
		m_observedRenderer->RemoveObserver( m_tag );
}

void iASegm3DViewData::SetDataToVisualize( vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	iASimpleTransferFunction tf(ctf, otf);
	if( !m_rendInitialized )
	{
		m_renderer->initialize( imgData, polyData );
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&tf, imgData));
		m_volumeRenderer->AddTo(m_renderer->GetRenderer());
		m_volumeRenderer->AddBoundingBoxTo(m_renderer->GetRenderer());
		m_rendInitialized = true;
	}
	else
	{
		m_renderer->reInitialize(imgData, polyData);
		m_volumeRenderer->SetImage(&tf, imgData);
	}
	m_wireMapper->SetInputData( polyData );
	UpdateColorCoding();
	vtkPolyDataMapper * mapper = m_renderer->GetPolyMapper();
	double sr[2];  mapper->GetScalarRange( sr );
	m_lut->SetRange( sr ); m_lut->SetTableRange( sr );
	mapper->SetLookupTable( m_lut );
	scalarBarWgt->GetScalarBarActor()->SetLookupTable( m_lut );
	scalarBarWgt->SetInteractor( m_renderer->GetInteractor() );
	volScalarBarWgt->GetScalarBarActor()->SetLookupTable( ctf );
	volScalarBarWgt->SetInteractor( m_renderer->GetInteractor() );
	LoadAndApplySettings();
}

void iASegm3DViewData::SetPolyData( vtkPolyData * polyData )
{
	if( !m_rendInitialized )
		return;
	m_wireMapper->SetInputData( polyData );
	m_renderer->setPolyData( polyData );
	vtkPolyDataMapper * mapper = m_renderer->GetPolyMapper();
	UpdateColorCoding();
	double sr[2];  mapper->GetScalarRange( sr );
	m_lut->SetRange( sr ); m_lut->SetTableRange( sr );
	mapper->SetLookupTable( m_lut );
	scalarBarWgt->GetScalarBarActor()->SetLookupTable( m_lut );
	scalarBarWgt->SetInteractor( m_renderer->GetInteractor() );
}

iARenderer * iASegm3DViewData::GetRenderer()
{
	return m_renderer;
}

void iASegm3DViewData::LoadAndApplySettings()
{
	// TODO: VOLUME: unify with mainwindow settings loading!
	QSettings settings( organisationName, applicationName );

	iARenderSettings renderSettings;
	iAVolumeSettings volumeSettings;

	renderSettings.ShowSlicers = settings.value("Renderer/rsShowSlicers", false).toBool();
	renderSettings.ShowHelpers = settings.value("Renderer/rsShowHelpers", true).toBool();
	renderSettings.ShowRPosition = settings.value("Renderer/rsShowRPosition", false).toBool();
	renderSettings.ParallelProjection = settings.value("Renderer/rsParallelProjection", false).toBool();
	renderSettings.BackgroundTop = settings.value("Renderer/rsBackgroundTop", "#8f8f8f").toString();
	renderSettings.BackgroundBottom = settings.value("Renderer/rsBackgroundBottom", "#8f8f8f").toString();

	volumeSettings.LinearInterpolation = settings.value("Renderer/rsLinearInterpolation", true).toBool();
	volumeSettings.Shading = settings.value("Renderer/rsShading", true).toBool();
	volumeSettings.SampleDistance = settings.value("Renderer/rsSampleDistance", 0.1).toDouble();
	volumeSettings.AmbientLighting = settings.value("Renderer/rsAmbientLighting", 0.2).toDouble();
	volumeSettings.DiffuseLighting = settings.value("Renderer/rsDiffuseLighting", 0.5).toDouble();
	volumeSettings.SpecularLighting = settings.value("Renderer/rsSpecularLighting", 0.7).toDouble();
	volumeSettings.SpecularPower = settings.value("Renderer/rsSpecularPower", 1).toDouble();
	volumeSettings.Mode = settings.value("Renderer/rsRenderMode", 0).toInt();
	
	m_renderer->ApplySettings(renderSettings);
	m_volumeRenderer->ApplySettings(volumeSettings);

	// TODO: VOLUME: apply volume/bounding box vis.!
	bool showBoundingBox = settings.value("Renderer/rsBoundingBox", true).toBool();
	bool showVolume = settings.value("PorosityAnalyser/GUI/ShowVolume", false).toBool();
	// m_renderer->GetOutlineActor()->SetVisibility(showBoundingBox);
	m_volumeRenderer->ShowBoundingBox(showBoundingBox);
	m_volumeRenderer->ShowVolume(showVolume);

	m_renderer->GetPolyActor()->SetVisibility( settings.value( "PorosityAnalyser/GUI/ShowSurface", false ).toBool() );
	m_wireActor->SetVisibility( settings.value( "PorosityAnalyser/GUI/ShowWireframe", false ).toBool() );
	m_renderer->GetPolyActor()->GetProperty()->SetSpecular( 0 );
	m_renderer->GetPolyActor()->GetProperty()->SetDiffuse( 0 );
	m_renderer->GetPolyActor()->GetProperty()->SetAmbient( 1 );
	scalarBarWgt->SetEnabled( settings.value( "PorosityAnalyser/GUI/ShowSurface", false ).toBool() );
	volScalarBarWgt->SetEnabled( settings.value( "PorosityAnalyser/GUI/ShowVolume", false ).toBool() );
}

iAFast3DMagicLensWidget * iASegm3DViewData::GetWidget()
{
	return m_wgt;
}

void iASegm3DViewData::ShowVolume( bool visible )
{
	m_volumeRenderer->ShowVolume(visible);
	volScalarBarWgt->SetEnabled( visible );
	m_renderer->update();
}

void iASegm3DViewData::ShowSurface( bool visible )
{
	m_renderer->GetPolyActor()->SetVisibility( visible );
	scalarBarWgt->SetEnabled( visible );
	m_renderer->update();
}

void iASegm3DViewData::ShowWireframe( bool visible )
{
	m_wireActor->SetVisibility( visible );
	m_renderer->update();
}

void iASegm3DViewData::UpdateColorCoding()
{
	vtkPolyData * pd = m_renderer->GetPolyMapper()->GetInput();
	if( !pd )
		return;
	double range[2]; pd->GetPointData()->GetScalars()->GetRange( range );
	double absRange = fabs( range[1] ) > fabs( range[0] ) ? fabs( range[1] ) : fabs( range[0] );
	absRange *= m_sensitivity;
	if( absRange > *m_rangeExt )
		*m_rangeExt = absRange;
}

void iASegm3DViewData::SetSensitivity( double sensitivity )
{
	m_sensitivity = sensitivity;
	UpdateColorCoding();
}

void iASegm3DViewData::UpdateRange()
{
	vtkPolyDataMapper * mapper = m_renderer->GetPolyMapper();
	mapper->SetScalarRange( -*m_rangeExt, *m_rangeExt );
	m_renderer->update();
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASegm3DView.h"

#include <defines.h>    // for organisationName / applicationName
#include <iALog.h>
#include <iAFast3DMagicLensWidget.h>
#include <iALUT.h>
#include <iARenderSettings.h>
#include <iATransferFunctionPtrs.h>
#include <iAVolumeRenderer.h>
#include <iAVolumeSettings.h>

#include <iARendererImpl.h>
#include <iARendererViewSync.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkLookupTable.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarRepresentation.h>
#include <vtkScalarBarWidget.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>

#include <QSettings>
#include <QHBoxLayout>
#include <QLabel>

iASegm3DView::iASegm3DView(QWidget* parent) :
	Segm3DViewContainer(parent),
	m_layout( new QHBoxLayout( wgtContainer ) ),
	m_renMgr( new iARendererViewSync ),
	m_range( 0.0 )
{}

void iASegm3DView::SetDataToVisualize( QList<vtkImageData*> imgData, QList<vtkPolyData*> polyData, QList<vtkPiecewiseFunction*> otf, QList<vtkColorTransferFunction*> ctf, QStringList slicerNames )
{
	for (auto i : imgData)
	{
		if (!i)
		{
			LOG(lvlError, "Image data is nullptr!");
			return;
		}
	}
	m_range = 0.0;
	m_renMgr->removeAll();
	for (iASegm3DViewData* sd: m_data)
	{
		m_layout->removeWidget( sd->GetWidget() );
		delete sd;
	}
	m_data.clear();

	for (QWidget* c: m_containerList)
	{
		m_layout->removeWidget( c );
		delete c;
	}
	m_containerList.clear();

	int sz = imgData.size();
	for (int i = 0; i < sz; ++i)
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
		m_renMgr->addToBundle( sd->GetRenderer()->renderer() );

		m_layout->addWidget( container );
	}
	for (iASegm3DViewData* sd: m_data)
	{
		sd->UpdateRange();
	}
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
	for (iASegm3DViewData* sd : m_data)
	{
		sd->UpdateRange();
	}
}

iASegm3DView::~iASegm3DView()
{
	for (iASegm3DViewData* sd : m_data)
	{
		delete sd;
	}
}

void iASegm3DView::ShowVolume( bool visible )
{
	for (iASegm3DViewData* sd : m_data)
	{
		sd->ShowVolume( visible );
	}
}

void iASegm3DView::ShowSurface( bool visible )
{
	for (iASegm3DViewData* sd : m_data)
	{
		sd->ShowSurface(visible);
	}
}

void iASegm3DView::SetSensitivity( double sensitivity )
{
	m_range = 0.0;
	for (iASegm3DViewData* sd : m_data)
	{
		sd->SetSensitivity( sensitivity );
	}
	for (iASegm3DViewData* sd: m_data)
	{
		sd->UpdateRange();
	}
}

void iASegm3DView::ShowWireframe( bool visible )
{
	for (iASegm3DViewData* sd : m_data)
	{
		sd->ShowWireframe( visible );
	}
}


iASegm3DViewData::iASegm3DViewData( double * rangeExt, QWidget * parent ) :
	m_rendInitialized( false ),
	m_axesTransform( vtkSmartPointer<vtkTransform>::New() ),
	m_observedRenderer( 0 ),
	m_tag( 0 ),
	m_wgt( new iAFast3DMagicLensWidget ),
	m_renderer(new iARendererImpl(parent, dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_wgt->renderWindow()) )),
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
	// TODO NEWIO: create own polymapper or use datasets!
	vtkPolyDataMapper* mapper;// = m_renderer->polyMapper();
	double sr[2];  mapper->GetScalarRange(sr);
	iALUT::BuildLUT( m_lut, sr, "Diverging blue-gray-red" );
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
	m_renderer->renderer()->AddActor( m_wireActor );
	m_renderer->setAxesTransform( m_axesTransform );

	QObject::connect(m_wgt, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	QObject::connect(m_wgt, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
}

iASegm3DViewData::~iASegm3DViewData()
{
	delete m_renderer;
	delete m_wgt;
}

void iASegm3DViewData::removeObserver()
{
	//is m_renderer deleted by Qt?
	if (m_observedRenderer)
	{
		m_observedRenderer->RemoveObserver(m_tag);
	}
}

void iASegm3DViewData::SetDataToVisualize( vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	if (!imgData)
	{
		LOG(lvlError, "Image data is nullptr!");
		return;
	}
	iATransferFunctionPtrs tf(ctf, otf);
	if( !m_rendInitialized )
	{
		// TODO NEWIO: create own polymapper or use datasets!
		//m_renderer->initialize( imgData, polyData );
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>::create(&tf, imgData);
		m_volumeRenderer->addTo(m_renderer->renderer());
		m_volumeRenderer->addBoundingBoxTo(m_renderer->renderer());
		m_rendInitialized = true;
	}
	else
	{
		// TODO NEWIO: create own polymapper or use datasets!
		//m_renderer->reInitialize(imgData, polyData);
		m_volumeRenderer->setImage(&tf, imgData);
	}
	m_wireMapper->SetInputData( polyData );
	UpdateColorCoding();
	// TODO NEWIO: create own polymapper or use datasets!
	vtkPolyDataMapper* mapper;// = m_renderer->polyMapper();
	double sr[2];  mapper->GetScalarRange( sr );
	m_lut->SetRange( sr ); m_lut->SetTableRange( sr );
	mapper->SetLookupTable( m_lut );
	scalarBarWgt->GetScalarBarActor()->SetLookupTable( m_lut );
	scalarBarWgt->SetInteractor( m_renderer->interactor() );
	volScalarBarWgt->GetScalarBarActor()->SetLookupTable( ctf );
	volScalarBarWgt->SetInteractor( m_renderer->interactor() );
	LoadAndApplySettings();
}

void iASegm3DViewData::SetPolyData( vtkPolyData * polyData )
{
	if (!m_rendInitialized)
	{
		return;
	}
	m_wireMapper->SetInputData( polyData );
	// TODO NEWIO: create own polymapper or use datasets!
	//m_renderer->setPolyData( polyData );
	vtkPolyDataMapper* mapper;// = m_renderer->polyMapper();
	UpdateColorCoding();
	double sr[2];  mapper->GetScalarRange( sr );
	m_lut->SetRange( sr ); m_lut->SetTableRange( sr );
	mapper->SetLookupTable( m_lut );
	scalarBarWgt->GetScalarBarActor()->SetLookupTable( m_lut );
	scalarBarWgt->SetInteractor( m_renderer->interactor() );
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
	renderSettings.ParallelProjection = true;
	renderSettings.BackgroundTop = "#8f8f8f"; //"#FFFFFF"
	renderSettings.BackgroundBottom = "#8f8f8f";

	volumeSettings.LinearInterpolation = settings.value("Renderer/rsLinearInterpolation", true).toBool();
	volumeSettings.Shading = settings.value("Renderer/rsShading", true).toBool();
	volumeSettings.SampleDistance = settings.value("Renderer/rsSampleDistance", 0.1).toDouble();
	volumeSettings.AmbientLighting = settings.value("Renderer/rsAmbientLighting", 0.2).toDouble();
	volumeSettings.DiffuseLighting = settings.value("Renderer/rsDiffuseLighting", 0.5).toDouble();
	volumeSettings.SpecularLighting = settings.value("Renderer/rsSpecularLighting", 0.7).toDouble();
	volumeSettings.SpecularPower = settings.value("Renderer/rsSpecularPower", 1).toDouble();
	volumeSettings.RenderMode = settings.value("Renderer/rsRenderMode", 0).toInt();

	bool slicerVisibility[3] = { false, false, false };
	m_renderer->applySettings(renderSettings, slicerVisibility);
	m_volumeRenderer->applySettings(volumeSettings);

	// TODO: VOLUME: apply volume/bounding box vis.!
	bool showBoundingBox = settings.value("Renderer/rsBoundingBox", true).toBool();
	bool showVolume = settings.value("FeatureAnalyzer/GUI/ShowVolume", false).toBool();
	// m_renderer->GetOutlineActor()->SetVisibility(showBoundingBox);
	m_volumeRenderer->showBoundingBox(showBoundingBox);
	m_volumeRenderer->showVolume(showVolume);
	
	// TODO NEWIO: create own polymapper or use datasets!
	//m_renderer->polyActor()->SetVisibility( settings.value( "FeatureAnalyzer/GUI/ShowSurface", false ).toBool() );
	m_wireActor->SetVisibility( settings.value( "FeatureAnalyzer/GUI/ShowWireframe", false ).toBool() );

	// TODO NEWIO: create own polymapper or use datasets!
	//m_renderer->polyActor()->GetProperty()->SetSpecular( 0 );
	//m_renderer->polyActor()->GetProperty()->SetDiffuse( 0 );
	//m_renderer->polyActor()->GetProperty()->SetAmbient( 1 );
	scalarBarWgt->SetEnabled( settings.value( "FeatureAnalyzer/GUI/ShowSurface", false ).toBool() );
	volScalarBarWgt->SetEnabled( settings.value( "FeatureAnalyzer/GUI/ShowVolume", false ).toBool() );

	m_renderer->renderer()->ResetCamera();

	vtkSmartPointer<vtkLight> light = vtkSmartPointer<vtkLight>::New();
	light->SetLightTypeToSceneLight();
	light->SetPosition( 0, 0, 1 );
	light->SetConeAngle( 45 );
	light->SetFocalPoint( 0,0,0 );
	light->SetDiffuseColor( 1, 1, 1 );
	light->SetAmbientColor( 1, 1, 1 );
	light->SetSpecularColor( 1, 1, 1 );
	m_renderer->renderer()->AddLight( light );
}

iAFast3DMagicLensWidget * iASegm3DViewData::GetWidget()
{
	return m_wgt;
}

void iASegm3DViewData::ShowVolume( bool visible )
{
	m_volumeRenderer->showVolume(visible);
	volScalarBarWgt->SetEnabled( visible );
	m_renderer->update();
}

void iASegm3DViewData::ShowSurface( bool visible )
{
	// TODO NEWIO: create own polymapper or use datasets!
	//m_renderer->polyActor()->SetVisibility( visible );
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
	// TODO NEWIO: create own polymapper or use datasets!
	vtkPolyData* pd;// = m_renderer->polyMapper()->GetInput();
	if (!pd)
	{
		return;
	}
	double range[2]; pd->GetPointData()->GetScalars()->GetRange( range );
	double absRange = fabs( range[1] ) > fabs( range[0] ) ? fabs( range[1] ) : fabs( range[0] );
	absRange *= m_sensitivity;
	if (absRange > * m_rangeExt)
	{
		*m_rangeExt = absRange;
	}
}

void iASegm3DViewData::SetSensitivity( double sensitivity )
{
	m_sensitivity = sensitivity;
	UpdateColorCoding();
}

void iASegm3DViewData::UpdateRange()
{
	// TODO NEWIO: create own polymapper or use datasets!
	vtkPolyDataMapper* mapper; // = m_renderer->polyMapper();
	mapper->SetScalarRange( -*m_rangeExt, *m_rangeExt );
	m_renderer->update();
}

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAAbstractMagicLensWidget.h"

#include "defines.h" // for DefaultMagicLensSize
#include "iAVtkVersion.h"

#include <QVTKInteractor.h>
#include <vtkActor2D.h>
#include <vtkCellArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolyLine.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>

#include <QEvent>
#include <QTouchEvent>

const double iAAbstractMagicLensWidget::OFFSET_VAL = 20.;

iAAbstractMagicLensWidget::iAAbstractMagicLensWidget( QWidget * parent /*= 0 */ ) :
	iAQVTKWidget(parent),
	m_lensRen{ vtkSmartPointer<vtkRenderer>::New( ) },
	m_GUIRen{ vtkSmartPointer<vtkRenderer>::New( ) },
	m_GUIActor { vtkSmartPointer<vtkActor2D>::New() },
	m_viewMode( iAAbstractMagicLensWidget::OFFSET ),
	m_magicLensEnabled(false)
{ }

iAAbstractMagicLensWidget::~iAAbstractMagicLensWidget( )
{ /* not impemented */ }

void iAAbstractMagicLensWidget::magicLensOn( )
{
	m_magicLensEnabled = true;
	setCursor( Qt::BlankCursor );
	renderWindow()->AddRenderer(m_lensRen);
	renderWindow()->AddRenderer(m_GUIRen);
	renderWindow()->Render();
}

void iAAbstractMagicLensWidget::magicLensOff( )
{
	m_magicLensEnabled = false;
	setCursor( Qt::ArrowCursor );
	renderWindow()->RemoveRenderer(m_lensRen);
	renderWindow()->RemoveRenderer(m_GUIRen);
	renderWindow()->Render();
}

bool iAAbstractMagicLensWidget::isMagicLensEnabled() const
{
	return m_magicLensEnabled;
}

void iAAbstractMagicLensWidget::setLensSize( int sizeX, int sizeY )
{
	m_size[0] = sizeX; m_size[1] = sizeY;
	m_halfSize[0] = .5 * sizeX; m_halfSize[1] = .5 * sizeY;
}

vtkRenderer * iAAbstractMagicLensWidget::getLensRenderer( )
{
	return m_lensRen.GetPointer();
}

void iAAbstractMagicLensWidget::setViewMode( ViewMode mode )
{
	m_viewMode = mode;
}

void iAAbstractMagicLensWidget::mouseMoveEvent( QMouseEvent * event )
{
	iAVtkWidget::mouseMoveEvent( event );
	int* pos = interactor()->GetEventPosition();
	m_pos[0] = pos[0]; m_pos[1] = pos[1];
	updateLens( );
	updateGUI( );
	emit mouseMoved();
	renderWindow()->Render();
}

bool iAAbstractMagicLensWidget::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd:
	{
		QTouchEvent* touchEvent = static_cast<QTouchEvent*>(event);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
#else
		QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->points();
#endif
		//LOG(lvlDebug, QString("event: %1, touchpoints: %2, state: %3")
		//	.arg(event->type() == QEvent::TouchBegin ? "TouchBegin" : (event->type() == QEvent::TouchEnd ? "TouchEnd" : "TouchUpdate") )
		//	.arg(touchPoints.count())
		//	.arg(touchEvent->touchPointStates())
		//);
		if (touchPoints.count() == 2)
		{
			if (touchEvent->touchPointStates() & Qt::TouchPointPressed)
			{
				emit touchStart();
			}
			// determine scale factor
			const auto & touchPoint0 = touchPoints.first();
			const auto & touchPoint1 = touchPoints.last();
			qreal currentScaleFactor = 
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
				QLineF(touchPoint0.startPos(), touchPoint1.startPos()).length() /
				QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
#else
				QLineF(touchPoint0.pressPosition(), touchPoint1.pressPosition()).length() /
				QLineF(touchPoint0.position(), touchPoint1.position()).length();
#endif
			//LOG(lvlDebug, QString("scale: %1").arg(currentScaleFactor));
			// ToDo - handle special cases, e.g. :
			//			- one touch point is moved
			//          - then other point is lifted
			//          - then another point is touched -> immediate zooming reaction due to move of pressPosition of "unlifted" point
			emit touchScale(currentScaleFactor);
			return true;
		}
		// other cases should be handled by default event handler, i.e. fall-through:
#if __cplusplus >= 201703L
		[[fallthrough]];
#endif
	}
	default:
		return iAVtkWidget::event(event);
	}
}

void iAAbstractMagicLensWidget::updateLens( )
{
	if( renderWindow( )->GetRenderers( )->GetNumberOfItems( ) <= 0 )
	{
		return;
	}
	double points[4];
	getViewportPoints( points );
	m_lensRen->SetViewport( points[0], points[1], points[2], points[3] );
}

void iAAbstractMagicLensWidget::updateGUI( )
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New( );
	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New( );

	switch( m_viewMode )
	{
	case ViewMode::CENTERED:
	{
		vtkSmartPointer<vtkPolyLine> line = vtkSmartPointer<vtkPolyLine>::New( );
		double p0[3] = { m_pos[0] - m_halfSize[0], m_pos[1] - m_halfSize[1], 0. };
		double p1[3] = { m_pos[0] - m_halfSize[0], m_pos[1] + m_halfSize[1], 0. };
		double p2[3] = { m_pos[0] + m_halfSize[0], m_pos[1] + m_halfSize[1], 0. };
		double p3[3] = { m_pos[0] + m_halfSize[0], m_pos[1] - m_halfSize[1], 0. };
		points->InsertNextPoint( p0 );
		points->InsertNextPoint( p1 );
		points->InsertNextPoint( p2 );
		points->InsertNextPoint( p3 );
		line->GetPointIds( )->SetNumberOfIds( 5 );
		for (int i = 0; i < 5; i++)
		{
			line->GetPointIds()->SetId(i, i % 4);
		}
		cells->InsertNextCell( line );
		break;
	}
	case ViewMode::OFFSET:
	{
		vtkSmartPointer<vtkPolyLine> leftRect = vtkSmartPointer<vtkPolyLine>::New( );
		vtkSmartPointer<vtkPolyLine> rightRect = vtkSmartPointer<vtkPolyLine>::New( );
		vtkSmartPointer<vtkPolyLine> line = vtkSmartPointer<vtkPolyLine>::New( );
		double p0[3] = { m_pos[0] - m_halfSize[0]			  , m_pos[1] - m_halfSize[1], 0. };	// left
		double p1[3] = { m_pos[0] - m_halfSize[0]			  , m_pos[1] + m_halfSize[1], 0. };	// left
		double p2[3] = { m_pos[0] + m_halfSize[0]			  , m_pos[1] + m_halfSize[1], 0. };	// left
		double p3[3] = { m_pos[0] + m_halfSize[0]			  , m_pos[1] - m_halfSize[1], 0. };	// left
		double p4[3] = { m_pos[0] - m_halfSize[0] + ( m_size[0] + OFFSET_VAL ), m_pos[1] - m_halfSize[1], 0. };	// right
		double p5[3] = { m_pos[0] - m_halfSize[0] + ( m_size[0] + OFFSET_VAL ), m_pos[1] + m_halfSize[1], 0. };	// right
		double p6[3] = { m_pos[0] + m_halfSize[0] + ( m_size[0] + OFFSET_VAL ), m_pos[1] + m_halfSize[1], 0. };	// right
		double p7[3] = { m_pos[0] + m_halfSize[0] + ( m_size[0] + OFFSET_VAL ), m_pos[1] - m_halfSize[1], 0. };	// right
		double p8[3] = { m_pos[0] + m_halfSize[0]			  , static_cast<double>( m_pos[1] )	  , 0. };	// line
		double p9[3] = { m_pos[0] - m_halfSize[0] + ( m_size[0] + OFFSET_VAL ), static_cast<double>( m_pos[1] ), 0. };	// line
		points->InsertNextPoint( p0 );
		points->InsertNextPoint( p1 );
		points->InsertNextPoint( p2 );
		points->InsertNextPoint( p3 );
		points->InsertNextPoint( p4 );
		points->InsertNextPoint( p5 );
		points->InsertNextPoint( p6 );
		points->InsertNextPoint( p7 );
		points->InsertNextPoint( p8 );
		points->InsertNextPoint( p9 );
		leftRect->GetPointIds( )->SetNumberOfIds( 5 );
		leftRect->GetPointIds( )->SetId( 0, 0 );
		leftRect->GetPointIds( )->SetId( 1, 1 );
		leftRect->GetPointIds( )->SetId( 2, 2 );
		leftRect->GetPointIds( )->SetId( 3, 3 );
		leftRect->GetPointIds( )->SetId( 4, 0 );
		rightRect->GetPointIds( )->SetNumberOfIds( 5 );
		rightRect->GetPointIds( )->SetId( 0, 4 );
		rightRect->GetPointIds( )->SetId( 1, 5 );
		rightRect->GetPointIds( )->SetId( 2, 6 );
		rightRect->GetPointIds( )->SetId( 3, 7 );
		rightRect->GetPointIds( )->SetId( 4, 4 );
		line->GetPointIds( )->SetNumberOfIds( 2 );
		line->GetPointIds( )->SetId( 0, 8 );
		line->GetPointIds( )->SetId( 1, 9 );
		cells->InsertNextCell( leftRect );
		cells->InsertNextCell( rightRect );
		cells->InsertNextCell( line );
		break;
	}
	default:
		break;
	}

	vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New( );
	polyData->SetPoints( points );
	polyData->SetLines( cells );
	// Setup actor and mapper
	vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New( );
	mapper->SetInputData( polyData );
	m_GUIActor->SetMapper( mapper );
}

// input points: xmin, ymin, xmax, ymax
void iAAbstractMagicLensWidget::getViewportPoints( double points[4] )
{
	int * winSize = renderWindow( )->GetSize( );
	switch( m_viewMode )
	{
	case ViewMode::CENTERED:
		points[0] = ( m_pos[0] - m_halfSize[0] ) / winSize[0];
		points[1] = ( m_pos[1] - m_halfSize[1] ) / winSize[1];
		points[2] = ( m_pos[0] + m_halfSize[0] ) / winSize[0];
		points[3] = ( m_pos[1] + m_halfSize[1] ) / winSize[1];
		break;
	case ViewMode::OFFSET:
		points[0] = ( m_pos[0] - m_halfSize[0] + ( m_size[0] + OFFSET_VAL ) ) / winSize[0];
		points[1] = ( m_pos[1] - m_halfSize[1] ) / winSize[1];
		points[2] = ( m_pos[0] + m_halfSize[0] + ( m_size[0] + OFFSET_VAL ) ) / winSize[0];
		points[3] = ( m_pos[1] + m_halfSize[1] ) / winSize[1];
		break;
	default:
		break;
	}
}


void iAAbstractMagicLensWidget::SetMainRenderWindow( vtkGenericOpenGLRenderWindow* renWin )
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	SetRenderWindow(renWin);
#else
	setRenderWindow(renWin);
#endif

	// TODO: VOLUME: move somewhere else?
	renWin->SetNumberOfLayers( 5 );

	m_lensRen->SetLayer( 0 );
	m_GUIRen->SetLayer( 1 );
	m_lensRen->InteractiveOff( );
	m_lensRen->SetBackground( 0.5, 0.5, 0.5 );
	m_GUIRen->InteractiveOff( );
	m_GUIRen->AddActor( m_GUIActor );
	m_GUIActor->GetProperty( )->SetLineWidth( 2. );
	m_GUIActor->GetProperty( )->SetColor( 1., 1., 0 );

	// default values
	setLensSize( DefaultMagicLensSize, DefaultMagicLensSize );
}

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
#include "iAMagicLens.h"

#include "iAFramedQVTKWidget2.h"

#include <QVTKInteractor.h>
#include <QVTKInteractorAdapter.h>
#include <QVTKWidget2.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

// TestMagicLens
#include "vtkImageGridSource.h"
#include "vtkLookupTable.h"
#include "vtkImageBlend.h"
#include "vtkThinPlateSplineTransform.h"
#include "vtkRenderWindow.h"
#include "vtkImageMapper.h"
#include "vtkActor2D.h"
#include "vtkPoints2D.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkSphereSource.h"
#include "vtkCubeSource.h"
#include "vtkProperty.h"
#include "qmath.h"
#include "vtkRegularPolygonSource.h"
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkMatrix4x4.h"
// End TestMagicLens

const int iAMagicLens::DEFAULT_SIZE = 120;
const int iAMagicLens::OFFSET_MODE_X_OFFSET = 30;

namespace
{
	const int CaptionFrameDistance = 0;
	const int CaptionFontSize = 13;
}

iAMagicLens::iAMagicLens() :
		m_qvtkWidget(0), 
		m_isEnabled(false), 
		m_splitPosition(0.65f),
		m_isInitialized(false),
		m_size(DEFAULT_SIZE)
{
	SetViewMode(CENTERED);
	m_ren = vtkSmartPointer<vtkRenderer>::New(); // Renderer
	m_cam = vtkSmartPointer<vtkCamera>::New();
	m_renWnd = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); // Render Window
	m_w2i = vtkSmartPointer<vtkWindowToImageFilter>::New();
	m_imageToColors = vtkSmartPointer<vtkImageMapToColors>::New();
	m_bgImageToColors = vtkSmartPointer<vtkImageMapToColors>::New();

	//Klara
	lensGrid = vtkSmartPointer<vtkImageGridSource >::New(); // the grid for the lens
	m_gridToColors = vtkSmartPointer<vtkImageMapToColors>::New(); // mapped grid
	m_blend = vtkSmartPointer<vtkImageBlend>::New();
	//endKlara

	m_cam->ParallelProjectionOn();

	m_ren->SetActiveCamera(m_cam);
	m_ren->ResetCamera();
	m_ren->InteractiveOff();

	m_bgImageActor = vtkSmartPointer<vtkImageActor>::New(); // actor for background
	m_bgImageActor->SetInputData(m_bgImageToColors->GetOutput());
	m_bgImageActor->GetMapper()->BorderOn();
	// for hiding the original image and only showing the overlayed image in the lens
	m_bgImageActor->SetOpacity( 0.0 );	// TestMagicLens

	m_imageActor = vtkSmartPointer<vtkImageActor>::New(); // actor for overlayed lens image
	m_imageActor->SetInputData(m_imageToColors->GetOutput());
	m_imageActor->GetMapper()->BorderOn();
	m_imageActor->SetOpacity(1.0); // opacity of the lens image

	//Klara
	m_gridImageActor = vtkSmartPointer<vtkImageActor>::New(); // actor for blended lens image
	m_gridImageActor->SetInputData(m_blend->GetOutput());
	m_gridImageActor->GetMapper()->BorderOn();
	m_gridImageActor->SetOpacity(1.0); // opacity of the grid

	m_gridReslice = vtkSmartPointer<vtkImageReslice>::New();
	m_gridMapper = vtkSmartPointer<vtkImageMapper>::New();
	m_gridActor = vtkSmartPointer<vtkActor2D>::New();
	m_gridActor->SetMapper(m_gridMapper);
	//m_gridActor->SetPosition(0.0, 0.0);
	//m_ren->AddActor(m_gridActor);
	//endKlara

/*
	// ORIENTATION / ROTATION FIX:
	// make orientation the same as in other image viewers:
	double orientation[3] = {
		180,
		0,
		0
	};
	m_imageActor->SetOrientation(orientation);
*/

	m_textActor = vtkSmartPointer<vtkTextActor>::New();
	m_textActor->GetTextProperty()->SetFontSize ( CaptionFontSize );
	m_textActor->SetPosition( GetFrameWidth() + CaptionFrameDistance, GetFrameWidth() + CaptionFrameDistance);
	m_ren->AddActor2D ( m_textActor );
	m_textActor->SetInput("");
	m_textActor->GetTextProperty()->SetColor ( 0.0,0.0,0.0 );
	m_textActor->GetTextProperty()->SetBackgroundColor(1.0, 1.0, 1.0);
	m_textActor->GetTextProperty()->SetBackgroundOpacity(0.5);

	m_ren->AddActor(m_bgImageActor); 
	m_ren->AddActor(m_imageActor); // displays the overlayed image in the lens
	//Klara
	//m_ren->AddActor(m_gridImageActor); // just for test reasons for the overlaid grid
	//endKlara
	
	m_renWnd->AddRenderer(m_ren);
	m_renWnd->DoubleBufferOn();
}


void iAMagicLens::SetCaption(std::string const & caption)
{
	m_textActor->SetInput(caption.c_str() );
}


void iAMagicLens::SetFrameWidth(qreal frameWidth)
{
	m_qvtkWidget->SetFrameWidth(frameWidth);
	m_textActor->SetPosition(GetFrameWidth() + CaptionFrameDistance, GetFrameWidth() + CaptionFrameDistance);
}

// width of the frame line
qreal iAMagicLens::GetFrameWidth() const
{
	if (m_qvtkWidget)
	{
		return m_qvtkWidget->GetFrameWidth();
	}
	else
	{
		return 2.0;
	}
}

iAMagicLens::~iAMagicLens()
{
}

void iAMagicLens::Render()
{
	if(m_isInitialized)
		m_renWnd->Render();
}

// shows or hides the magic lense on the sliced image
void iAMagicLens::SetEnabled( bool isEnabled )
{
	m_isEnabled = isEnabled;
	if(m_isEnabled)
		m_qvtkWidget->show();
	else
		m_qvtkWidget->hide();
}

void iAMagicLens::SetRenderWindow( vtkGenericOpenGLRenderWindow * renWnd )
{
	m_qvtkWidget->SetRenderWindow(renWnd);
}

void iAMagicLens::SetGeometry( QRect & rect )
{
	m_viewedRect = rect;
	QRect offsetRect = QRect(	rect.x() + m_offset[0], 
								rect.y() + m_offset[1],
								rect.width(), rect.height());
	
	if(m_viewMode == SIDE_BY_SIDE)
	{
		int splitOffset = GetSplitOffset();
		offsetRect = QRect(	offsetRect.x()	+ splitOffset, offsetRect.y(),
							rect.width()	- splitOffset, rect.height());
	}

	m_qvtkWidget->setGeometry(offsetRect);
}

void iAMagicLens::InitWidget( QWidget * parent /*= NULL*/, const QGLWidget * shareWidget/*=0*/, Qt::WindowFlags f /*= 0*/ )
{
	if(m_qvtkWidget)
		delete m_qvtkWidget;
	m_qvtkWidget = new iAFramedQVTKWidget2( parent, shareWidget, f );
	m_qvtkWidget->hide();
	m_qvtkWidget->setEnabled(false);
	m_qvtkWidget->SetRenderWindow(m_renWnd);
	m_qvtkWidget->SetCrossHair(m_viewMode == OFFSET);
	
	QRect rect = QRect(0, 0, 100, 100);
	SetGeometry( rect );

	/*Klara
	int x = parent->geometry().x();
	int y = parent->geometry().y();
	int width = parent->width();
	int height = parent->height();

	lensGrid->SetGridSpacing(10, 10, 0);
	lensGrid->SetGridOrigin(0,0,0);
	lensGrid->SetDataExtent(0, width, 0, height, 0, 0);
	lensGrid->SetLineValue(4095);
	lensGrid->SetFillValue(0);
	lensGrid->SetDataScalarTypeToUnsignedChar();
	//endKlara */

	m_renWnd->GetInteractor()->Disable();
	//m_renWnd->OffScreenRenderingOn();
	m_w2i->SetInput(m_renWnd);
}


void iAMagicLens::SetScaleCoefficient( double scaleCoefficient )
{
	m_scaleCoefficient = scaleCoefficient;
} 

void iAMagicLens::UpdateCamera( double focalPt[3], vtkCamera * cam )
{
	//set focal point and corresponding scale
	m_cam->SetFocalPoint(focalPt);
	m_cam->SetParallelScale(cam->GetParallelScale()*m_scaleCoefficient);
	m_cam->SetRoll(cam->GetRoll());

	//set camera position
	double dir[3]; cam->GetDirectionOfProjection(dir);
	//vtkMath::MultiplyScalar(dir, cam->GetDistance());	//causes display error
	double res[3];
	vtkMath::Subtract(focalPt, dir, res);
	m_cam->SetPosition(res);
}

void iAMagicLens::Repaint()
{
	m_qvtkWidget->repaint();
}

const QObject * iAMagicLens::GetQObject() const
{
	return m_qvtkWidget;
}

void iAMagicLens::Frame()
{
	m_renWnd->Frame();
}

bool iAMagicLens::Enabled()
{
	return m_isEnabled;
}

void iAMagicLens::SetPaintingLocked( bool isLocked )
{
	m_qvtkWidget->setAttribute(Qt::WA_PaintOnScreen, isLocked);
}

void iAMagicLens::SetViewMode( ViewMode mode )
{
	m_viewMode = mode;
	if (m_qvtkWidget)
	{
		m_qvtkWidget->SetCrossHair(m_viewMode == OFFSET);
	}
	UpdateOffset();
	UpdateShowFrame();
}

void iAMagicLens::UpdateOffset()
{
	switch (m_viewMode)
	{
	case CENTERED:
		m_offset[0] = m_offset[1] = 0; 
		break;
	case OFFSET:
		m_offset[0] = GetOffset(0);	m_offset[1] = GetOffset(1);
		break;
	case SIDE_BY_SIDE:
		m_offset[0] = m_offset[1] = 0;
		break;
	}
}

// shows the image in the lens either next to the part of the image focused in the lens,
// or the lens frames the part of the image it is currently hovering over
void iAMagicLens::UpdateShowFrame()
{
	if (m_viewMode == SIDE_BY_SIDE)
	{
		SetShowFrame(iAFramedQVTKWidget2::LEFT_SIDE);
	}
	else
	{
		SetShowFrame(iAFramedQVTKWidget2::FRAMED);
	}
}

iAMagicLens::ViewMode iAMagicLens::GetViewMode() const
{
	return m_viewMode;
}

QRect iAMagicLens::GetViewRect() const
{
	return m_viewedRect;
}

// sets a frame for the lense: either framed, not framed, or framed on the left side
// of the original image
void iAMagicLens::SetShowFrame( iAFramedQVTKWidget2::FrameStyle frameStyle )
{
	if(m_qvtkWidget)
		m_qvtkWidget->SetFrameStyle(frameStyle);
}

int iAMagicLens::GetSplitOffset() const
{
	return qRound( m_viewedRect.width() * (1.f - m_splitPosition) );
}

int iAMagicLens::GetCenterSplitOffset() const
{
	return qRound( 0.5 * m_viewedRect.width() * (1.f - m_splitPosition) );
}

// sets the input for the magic lens that is shown on top of the original image
void iAMagicLens::SetInput( vtkImageReslice * reslicer, vtkScalarsToColors * cTF,
	vtkImageReslice * bgReslice, vtkScalarsToColors* bgCTF )
{
	if (!reslicer->GetInput())
	{
		return;
	}

	m_imageToColors->SetInputConnection( reslicer->GetOutputPort() );
	m_imageToColors->SetLookupTable( cTF );
	m_imageToColors->Update();

	m_bgImageToColors->SetInputConnection(bgReslice->GetOutputPort());
	m_bgImageToColors->SetLookupTable( bgCTF);
	m_bgImageToColors->Update();

	lensGrid->SetGridSpacing(16, 16, 0);
	lensGrid->SetGridOrigin(0, 0, 0);
	lensGrid->SetDataExtent(0, reslicer->GetInformationInput()->GetExtent()[1], /*x-dir*/
		0, reslicer->GetInformationInput()->GetExtent()[1], /*y-dir*/
		0, 0); /*z-dir*/
	lensGrid->SetLineValue(4000);
	lensGrid->SetFillValue(0.0);
	lensGrid->SetDataScalarTypeToUnsignedChar();
	lensGrid->Update();

	vtkSmartPointer<vtkLookupTable> table =
		vtkSmartPointer<vtkLookupTable >::New();
	table->SetTableRange(0, 1);
	table->SetAlphaRange(0.0, 1.0);
	table->SetHueRange(0.15, 0.15);
	table->SetSaturationRange(1, 1);
	table->SetValueRange(0, 1);
	table->Build();

	vtkSmartPointer<vtkLookupTable> lookupTable =
		vtkSmartPointer<vtkLookupTable>::New();
	lookupTable->SetNumberOfTableValues(2);
	double rgbaGrid[] = { 0.0, 255.0, 50, 0.8 }; // green
	double rgbaBg[] = { 0.0, 0.0, 0.0, 0.0 }; // transparent
	lookupTable->SetTableValue(0, rgbaGrid);
	lookupTable->SetTableValue(1, rgbaBg);
	lookupTable->Build();
	
	m_gridToColors->SetInputConnection(lensGrid->GetOutputPort());
	m_gridToColors->SetLookupTable(table);
	m_gridToColors->PassAlphaToOutputOn();
	m_gridToColors->Update();

	m_gridToColors->SetLookupTable(table);
	m_blend->SetOpacity(0, 0.5);
	m_blend->SetOpacity(1, 0.5);
	m_blend->AddInputConnection(0, m_imageToColors->GetOutputPort());
	m_blend->AddInputConnection(0, m_gridToColors->GetOutputPort());
	m_blend->Update();

	m_gridReslice->SetInputConnection(m_blend->GetOutputPort());
	m_gridReslice->SetInterpolationModeToLinear();

	m_gridMapper->SetInputConnection(m_gridReslice->GetOutputPort());
	m_gridMapper->SetColorWindow(255.0);
	m_gridMapper->SetColorLevel(90);	// lightness
	m_gridMapper->SetZSlice(0);

	// magic lens is initialized
	m_isInitialized = true;

	// TestMagicLens

	// Create yellow grid
	//GetInformationInput() <- returns vtkImageData (default Spacing, Origin, WholeExtent of the output will be copied)
	//GetExtent() <- set of 6 integers, says what the first and last pixel indices are in each of the three directions
	int *extents = reslicer->GetInformationInput()->GetExtent();
	vtkSmartPointer<vtkImageGridSource> imageGrid =
		vtkSmartPointer<vtkImageGridSource >::New();
	imageGrid->SetGridSpacing( 15, 15, 0 );
	imageGrid->SetGridOrigin( 0, 0, 0 );
	imageGrid->SetDataExtent( 0, extents[1], 0, extents[3], 0, extents[5] );
	imageGrid->SetDataScalarTypeToUnsignedChar();

	

	m_TestAlpha = vtkSmartPointer<vtkImageMapToColors >::New();
	m_TestAlpha->SetInputConnection( imageGrid->GetOutputPort() );
	m_TestAlpha->SetLookupTable( table );

	// Blend yellow grid and reslicer image
	m_TestBlend = vtkSmartPointer<vtkImageBlend >::New();
	m_TestBlend->AddInputConnection( 0, m_imageToColors->GetOutputPort() );
	m_TestBlend->AddInputConnection( 0, m_TestAlpha->GetOutputPort() );

	// External Renderer and Renderer window
	m_TestRenderer = vtkSmartPointer<vtkRenderer>::New();
	m_TestRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();

	//Clear outdated circles and actors 
	for ( int i = 0; i < circle1ActList.length(); ++i )
	{
		m_TestRenderer->RemoveActor2D( circle1ActList.at( i ) );
		m_TestRenderer->RemoveActor2D( textAct1List.at( i ));
		
	}
	circle1List.clear();
	circle1ActList.clear();
	textAct1List.clear();
	for ( int i = 0; i < circle2ActList.length(); ++i )
	{
		m_TestRenderer->RemoveActor2D( circle2ActList.at( i ) );
		m_TestRenderer->RemoveActor2D( textAct2List.at( i ) );
		
	}
	circle2List.clear();
	circle2ActList.clear();
	textAct2List.clear();

	// Thin plate spline transform
	m_transform = vtkSmartPointer< vtkThinPlateSplineTransform >::New();
	m_transform->SetBasisToR2LogR();

	// Create image for the external reslicer and renderer window 
	m_TestReslice = vtkSmartPointer<vtkImageReslice >::New();
	m_TestReslice->SetInputConnection( m_TestBlend->GetOutputPort() );
	m_TestReslice->SetInterpolationModeToLinear();

	m_TestMap = vtkSmartPointer<vtkImageMapper >::New();
	m_TestMap->SetInputConnection( m_TestReslice->GetOutputPort() );
	m_TestMap->SetColorWindow( 255.0 );
	m_TestMap->SetColorLevel( 90 );	// lightness
	m_TestMap->SetZSlice( 0 );

	m_TestMapAct = vtkSmartPointer< vtkActor2D >::New();
	m_TestMapAct->SetMapper( m_TestMap );
	m_TestMapAct->SetPosition( 0.0, 0.0 );
	m_TestRenderer->AddActor( m_TestMapAct );

	// Transform landmarks
	m_p1 = vtkSmartPointer< vtkPoints >::New();
	m_p1->SetNumberOfPoints( 24 );
	m_p2 = vtkSmartPointer< vtkPoints >::New();
	m_p2->SetNumberOfPoints( 24 );


	//Klara 
	fisheye = vtkSmartPointer<vtkRegularPolygonSource>::New();
	vtkSmartPointer<vtkPolyDataMapper2D> fisheyeMapper;
	vtkSmartPointer<vtkActor2D> fisheyeActor;

	fisheye->SetNumberOfSides(60);
	fisheye->SetRadius(70); // or 30 ?
	fisheye->GeneratePolygonOff(); // just outlines!

	fisheyeMapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
	fisheyeMapper->SetInputConnection(fisheye->GetOutputPort());

	fisheyeActor = vtkSmartPointer<vtkActor2D>::New();
	fisheyeActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
	fisheyeActor->GetProperty()->SetOpacity(1.0);
	fisheyeActor->GetProperty()->SetLineStipplePattern(0xf0f0);
	fisheyeActor->GetProperty()->SetLineStippleRepeatFactor(1);
	fisheyeActor->SetMapper(fisheyeMapper);

	m_TestRenderer->AddActor2D(fisheyeActor);
	//end Klara

	// Create circle actors (green and red) to show the transform landmarks
	for ( int i = 0; i < m_p1->GetNumberOfPoints(); ++i )
	{
		// Create a sphere and its associated mapper and actor.
		vtkSmartPointer<vtkRegularPolygonSource> circle =
			vtkSmartPointer<vtkRegularPolygonSource>::New();

		circle->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
		circle->SetNumberOfSides( 50 );
		circle->SetRadius( 1 );

		vtkSmartPointer<vtkPolyDataMapper2D> circleMapper =
			vtkSmartPointer<vtkPolyDataMapper2D>::New();
		circleMapper->SetInputConnection( circle->GetOutputPort() );

		vtkSmartPointer<vtkActor2D> circleActor =
			vtkSmartPointer<vtkActor2D>::New();
		circleActor->GetProperty()->SetColor( 0.0, 1.0, 0.0 );
		circleActor->GetProperty()->SetOpacity( 1.0 );
		circleActor->SetMapper( circleMapper );

		vtkSmartPointer<vtkTextActor> textActor =
			vtkSmartPointer<vtkTextActor>::New();
		textActor->GetTextProperty()->SetFontSize( 11 );
		textActor->GetTextProperty()->SetColor( 0.0, 1.0, 0.0 );
		textActor->GetTextProperty()->SetOpacity( 1.0 );

		circle1List.append( circle );
		circle1ActList.append( circleActor );
		textAct1List.append( textActor );
		m_TestRenderer->AddActor2D( textActor );
		m_TestRenderer->AddActor2D( circleActor );
		
	}
	for ( int i = 0; i < m_p2->GetNumberOfPoints(); ++i )
	{
		vtkSmartPointer<vtkRegularPolygonSource> circle =
			vtkSmartPointer<vtkRegularPolygonSource>::New();

		circle->GeneratePolygonOff(); // Uncomment this line to generate only the outline of the circle
		circle->SetNumberOfSides( 50 );
		circle->SetRadius( 3 );

		vtkSmartPointer<vtkPolyDataMapper2D> circleMapper =
			vtkSmartPointer<vtkPolyDataMapper2D>::New();
		circleMapper->SetInputConnection( circle->GetOutputPort() );

		vtkSmartPointer<vtkActor2D> circleActor =
			vtkSmartPointer<vtkActor2D>::New();
		circleActor->GetProperty()->SetColor( 1.0, 0.0, 0.0 );
		circleActor->GetProperty()->SetOpacity( 1.0 );
		circleActor->SetMapper( circleMapper );

		vtkSmartPointer<vtkTextActor> textActor =
			vtkSmartPointer<vtkTextActor>::New();
		textActor->GetTextProperty()->SetFontSize( 11 );
		textActor->GetTextProperty()->SetColor( 1.0, .0, 0.0 );
		textActor->GetTextProperty()->SetOpacity( 1.0 );

		circle2List.append( circle );
		circle2ActList.append( circleActor );
		textAct2List.append( textActor );
		//m_TestRenderer->AddActor2D( textActor );
		m_TestRenderer->AddActor2D( circleActor );
		
	}
	// external render windows for every axis
	if ( reslicer->GetResliceAxes()->GetElement(0, 0) == 1 &&
		 reslicer->GetResliceAxes()->GetElement( 1, 1 ) == 1)	// Z-axis
		 m_TestRenderWindow->SetSize( extents[1], extents[3] );
	else if ( reslicer->GetResliceAxes()->GetElement( 0, 0 ) == 1 &&
		 reslicer->GetResliceAxes()->GetElement( 1, 0 ) == 0 )	// Y-axis
		 m_TestRenderWindow->SetSize( extents[1], extents[5] );
	else if ( reslicer->GetResliceAxes()->GetElement( 0, 0 ) == 0 &&	
		 reslicer->GetResliceAxes()->GetElement( 1, 0 ) == 1 )	// X-axis
		 m_TestRenderWindow->SetSize( extents[3], extents[5] );
	m_TestRenderWindow->AddRenderer( m_TestRenderer );
	m_TestRenderWindow->Render();
	// End TestMagicLens
}

// TestMagicLens
void iAMagicLens::UpdateTransform( double focalPt[3], vtkImageReslice * reslicer )
{
	int xOffset = focalPt[0];	// Mouse cursor X-position
	int yOffset = focalPt[1];	// Mouse cursor Y-position

	fisheye->SetCenter(xOffset, yOffset, 0);

	//int extent[6] = { i_min, i_max, j_min, j_max, k_min, k_max };
	int *extent = reslicer->GetInformationInput()->GetExtent();
	double *spacing = reslicer->GetInformationInput()->GetSpacing();
	double *origin = reslicer->GetInformationInput()->GetOrigin();

	double bounds[6] = { extent[0] * spacing[0] + origin[0], extent[1] * spacing[0] + origin[0],
		extent[2] * spacing[1] + origin[1], extent[3] * spacing[1] + origin[1],
		extent[4] * spacing[2] + origin[2], extent[5] * spacing[2] + origin[2] };

	// points clockwise from (x_min, y_min) 
	m_p1->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
												// left border points
	m_p1->SetPoint(1, bounds[0], bounds[3] / 2/2, 0);
	m_p1->SetPoint(2, bounds[0], bounds[3] / 2, 0);
	m_p1->SetPoint(3, bounds[0], 3* bounds[3] /2 /2, 0);

	m_p1->SetPoint(4, bounds[0], bounds[3], 0); //x_min, y_max, top left
												 // top border points
	m_p1->SetPoint(5, (0.25)*bounds[1], bounds[3], 0);
	m_p1->SetPoint(6, (0.5)*bounds[1], bounds[3], 0);
	m_p1->SetPoint(7, (0.75)*bounds[1], bounds[3], 0);

	m_p1->SetPoint(8, bounds[1], bounds[3], 0); //x_max, y_max, top right
												// right border points
	m_p1->SetPoint(9, bounds[1], (0.75)*bounds[3], 0);
	m_p1->SetPoint(10, bounds[1], (0.5)*bounds[3], 0);
	m_p1->SetPoint(11, bounds[1], (0.25)*bounds[3], 0);

	m_p1->SetPoint(12, bounds[1], bounds[2], 0); //x_max, y_min, bottom right
	// bottom border points
	m_p1->SetPoint(13, (0.75)*bounds[1], bounds[2], 0);
	m_p1->SetPoint(14, (0.5)*bounds[1], bounds[2], 0);
	m_p1->SetPoint(15, (0.25)*bounds[1], bounds[2], 0);

	m_p2->SetNumberOfPoints(24);
	m_p2->SetPoint(0, bounds[0], bounds[2], 0); //x_min, y_min, bottom left
												// left border points
	m_p2->SetPoint(1, bounds[0], (0.25)*bounds[3], 0);
	m_p2->SetPoint(2, bounds[0], (0.5)*bounds[3], 0);
	m_p2->SetPoint(3, bounds[0], (0.75)*bounds[3], 0);

	m_p2->SetPoint(4, bounds[0], bounds[3], 0); //x_min, y_max, top left
												// top border points
	m_p2->SetPoint(5, (0.25)*bounds[1], bounds[3], 0);
	m_p2->SetPoint(6, (0.5)*bounds[1], bounds[3], 0);
	m_p2->SetPoint(7, (0.75)*bounds[1], bounds[3], 0);

	m_p2->SetPoint(8, bounds[1], bounds[3], 0); //x_max, y_max, top right
												// right border points
	m_p2->SetPoint(9, bounds[1], (0.75)*bounds[3], 0);
	m_p2->SetPoint(10, bounds[1], (0.5)*bounds[3], 0);
	m_p2->SetPoint(11, bounds[1], (0.25)*bounds[3], 0);

	m_p2->SetPoint(12, bounds[1], bounds[2], 0); //x_max, y_min, bottom right
												 // bottom border points
	m_p2->SetPoint(13, (0.75)*bounds[1], bounds[2], 0);
	m_p2->SetPoint(14, (0.5)*bounds[1], bounds[2], 0);
	m_p2->SetPoint(15, (0.25)*bounds[1], bounds[2], 0); //*/
	

	
	// "Outer" transform landmarks 
	/*m_p1->SetPoint( 0, 0, 0, 0 );
	
	m_p1->SetPoint( 1, 0, 599 / 2 / 2, 0 );
	m_p1->SetPoint( 2, 0, 599 / 2, 0 );
	m_p1->SetPoint( 3, 0, 3 * 599 / 2 / 2, 0 );
	
	m_p1->SetPoint( 4, 0, 599, 0 );
	
	m_p1->SetPoint( 5, 569 / 2 / 2, 599, 0 );
	m_p1->SetPoint( 6, 569 / 2, 599, 0 );
	m_p1->SetPoint( 7, 3 * 569 / 2 / 2, 599, 0 );
	
	m_p1->SetPoint( 8, 569, 599, 0 );
	
	m_p1->SetPoint( 9, 569, 3 * 599 / 2 / 2, 0 );
	m_p1->SetPoint( 10, 569, 599 / 2, 0 );
	m_p1->SetPoint( 11, 569, 599 / 2 / 2, 0 );
	
	m_p1->SetPoint( 12, 569, 0, 0 );
	
	m_p1->SetPoint( 13, 3 * 569 / 2 / 2, 0, 0 );
	m_p1->SetPoint( 14, 569 / 2, 0, 0 );
	m_p1->SetPoint( 15, 569 / 2 / 2, 0, 0 );

	m_p2->SetNumberOfPoints( 24 );
	m_p2->SetPoint( 0, 0, 0, 0 );
	
	m_p2->SetPoint( 1, 0, 599 / 2 / 2, 0 );
	m_p2->SetPoint( 2, 0, 599 / 2, 0 );
	m_p2->SetPoint( 3, 0, 3 * 599 / 2 / 2, 0 );
	
	m_p2->SetPoint( 4, 0, 599, 0 );
	
	m_p2->SetPoint( 5, 569 / 2 / 2, 599, 0 );
	m_p2->SetPoint( 6, 569 / 2, 599, 0 );
	m_p2->SetPoint( 7, 3 * 569 / 2 / 2, 599, 0 );
	
	m_p2->SetPoint( 8, 569, 599, 0 );
	
	m_p2->SetPoint( 9, 569, 3 * 599 / 2 / 2, 0 );
	m_p2->SetPoint( 10, 569, 599 / 2, 0 );
	m_p2->SetPoint( 11, 569, 599 / 2 / 2, 0 );
	
	m_p2->SetPoint( 12, 569, 0, 0 );
	
	m_p2->SetPoint( 13, 3 * 569 / 2 / 2, 0, 0 );
	m_p2->SetPoint( 14, 569 / 2, 0, 0 );
	m_p2->SetPoint( 15, 569 / 2 / 2, 0, 0 ); //*/

	// "Circle" transform landmarks
	int pointsCount = 8;
	for ( int i = 16; i < 16 + pointsCount; ++i )
	{
	
		double radius = 30.0;
		double xCoordCircle = radius * std::cos( i * ( 360 / pointsCount ) * M_PI / 180 );
		double yCoordCircle = radius * std::sin( i * ( 360 / pointsCount ) * M_PI / 180 );
		m_p1->SetPoint( i, xOffset + xCoordCircle, yOffset + yCoordCircle, 0 );	

		radius = 50.0;
		xCoordCircle = radius * std::cos( i * ( 360 / pointsCount ) * M_PI / 180 );
		yCoordCircle = radius * std::sin( i * ( 360 / pointsCount ) * M_PI / 180 );
		m_p2->SetPoint( i, xOffset + xCoordCircle, yOffset + yCoordCircle, 0 );
	}

	// Set position and text for green circle1 actors
	for ( int i = 0; i < m_p1->GetNumberOfPoints(); ++i )
	{
		circle1List.at(i)->SetCenter( m_p1->GetPoint( i )[0], m_p1->GetPoint( i )[1], 0 );
		//textAct1List.at( i )->SetInput( vtkStdString( QString::number( i ).toStdString()
		//	+ ", " + QString::number( m_p1->GetPoint( i )[0] ).toStdString()
		//	+ ", " + QString::number( m_p1->GetPoint( i )[1] ).toStdString() ).c_str() );
		//textAct1List.at(i)->SetDisplayPosition( m_p1->GetPoint( i )[0], m_p1->GetPoint( i )[1] );
	}

	// Set position and text for red circle2 actors
	for ( int i = 0; i < m_p2->GetNumberOfPoints(); ++i )
	{
		circle2List.at( i )->SetCenter( m_p2->GetPoint( i )[0], m_p2->GetPoint( i )[1], 0 );
		//textAct2List.at( i )->SetInput( vtkStdString( QString::number( i ).toStdString()
		//	+ ", " + QString::number( m_p2->GetPoint( i )[0] ).toStdString()
		//	+ ", " + QString::number( m_p2->GetPoint( i )[1] ).toStdString() ).c_str() );
		//textAct2List.at( i )->SetDisplayPosition( m_p2->GetPoint( i )[0], m_p2->GetPoint( i )[1] + 10 );
	}

	m_transform->SetSourceLandmarks( m_p2 ); // red
	m_transform->SetTargetLandmarks( m_p1 );  // green
	
	// Apply transform to internal reslicers and renderer windows 
	//reslicer->SetResliceTransform( m_transform );

	// Apply transform to grid reslicer (if that works at all)
	//m_gridReslice->SetResliceTransform(m_transform);
	
	UpdateColors();
	
	//m_gridReslice->Update();
	//m_renWnd->Render();
	
	// Apply transform to external (separated) reslicers and renderer windows 
	m_TestReslice->SetResliceTransform( m_transform );
	m_TestRenderWindow->Render();
}
//End TestMagicLens

void iAMagicLens::UpdateColors()
{
	if (m_isInitialized)
	{
		m_imageToColors->Update();
		m_bgImageToColors->Update();
	}
}


int iAMagicLens::GetSize() const
{
	return m_size;
}


void iAMagicLens::SetSize(int newSize)
{
	if (newSize < 40)
	{
		return;
	}
	m_size = newSize;
	UpdateOffset();
}

int iAMagicLens::GetOffset(int idx) const
{
	switch(idx)
	{
	case 0:
		return m_size + OFFSET_MODE_X_OFFSET;
	default:
		return 0;
	}
}

void iAMagicLens::SetInterpolate(bool on)
{
	m_imageActor->SetInterpolate(on);
}

void iAMagicLens::SetOpacity(double opacity)
{
	m_imageActor->SetOpacity(opacity);
}

double iAMagicLens::GetOpacity()
{
	return m_imageActor->GetOpacity();
}

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
	m_ren = vtkSmartPointer<vtkRenderer>::New();
	m_cam = vtkSmartPointer<vtkCamera>::New();
	m_renWnd = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	m_w2i = vtkSmartPointer<vtkWindowToImageFilter>::New();
	m_imageToColors = vtkSmartPointer<vtkImageMapToColors>::New();
	m_bgImageToColors = vtkSmartPointer<vtkImageMapToColors>::New();

	m_cam->ParallelProjectionOn();

	m_ren->SetActiveCamera(m_cam);
	m_ren->ResetCamera();
	m_ren->InteractiveOff();

	m_bgImageActor = vtkSmartPointer<vtkImageActor>::New();
	m_bgImageActor->SetInputData(m_bgImageToColors->GetOutput());
	m_bgImageActor->GetMapper()->BorderOn();

	m_imageActor = vtkSmartPointer<vtkImageActor>::New();
	m_imageActor->SetInputData(m_imageToColors->GetOutput());
	m_imageActor->GetMapper()->BorderOn();
	m_imageActor->SetOpacity(1.0);
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
	m_ren->AddActor(m_imageActor);
	
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
	m_qvtkWidget = new iAFramedQVTKWidget2(parent, shareWidget, f);
	m_qvtkWidget->hide();
	m_qvtkWidget->setEnabled(false);
	m_qvtkWidget->SetRenderWindow(m_renWnd);
	m_qvtkWidget->SetCrossHair(m_viewMode == OFFSET);
	QRect rect = QRect(0, 0, 100, 100);
	SetGeometry( rect );
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

	m_isInitialized = true;
}


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

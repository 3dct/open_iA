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
#include "iAMagicLens.h"

#include "defines.h"
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
const int iAMagicLens::OFFSET_MODE_X_OFFSET = 10;

namespace
{
	const int CaptionFrameDistance = 0;
	const int CaptionFontSize = 13;
}


LensData::LensData():
	m_qvtkWidget(0)
{}


LensData::LensData(QWidget * parent, const QGLWidget * shareWidget, Qt::WindowFlags f, bool interpolate, bool enabled):
	m_qvtkWidget(new iAFramedQVTKWidget2(parent, shareWidget, f)),
	m_ren(vtkSmartPointer<vtkRenderer>::New()),
	m_cam(vtkSmartPointer<vtkCamera>::New()),
	m_renWnd(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),
	m_imageToColors(vtkSmartPointer<vtkImageMapToColors>::New()),
	m_imageActor(vtkSmartPointer<vtkImageActor>::New()),
	m_bgImageToColors(vtkSmartPointer<vtkImageMapToColors>::New()),
	m_bgImageActor(vtkSmartPointer<vtkImageActor>::New()),
	m_textActor(vtkSmartPointer<vtkTextActor>::New())
{
	m_bgImageActor->SetInputData(m_bgImageToColors->GetOutput());
	m_bgImageActor->GetMapper()->BorderOn();
	m_bgImageActor->SetOpacity(1.0);
	m_bgImageActor->SetInterpolate(interpolate);

	m_imageActor->SetInputData(m_imageToColors->GetOutput());
	m_imageActor->GetMapper()->BorderOn();
	m_imageActor->SetOpacity(1.0); // opacity of the lens image
	m_imageActor->SetInterpolate(interpolate);
/*
	// ORIENTATION / ROTATION FIX:
	double orientation[3] = {180, 0, 0};
	m_imageActor->SetOrientation(orientation);
*/
	m_textActor->GetTextProperty()->SetColor ( 0.0,0.0,0.0 );
	m_textActor->GetTextProperty()->SetBackgroundColor(1.0, 1.0, 1.0);
	m_textActor->GetTextProperty()->SetBackgroundOpacity(0.5);
	m_textActor->GetTextProperty()->SetFontSize(CaptionFontSize);
	double textMargin = m_qvtkWidget->GetFrameWidth() + CaptionFrameDistance;
	m_textActor->SetPosition(textMargin, textMargin);

	m_cam->ParallelProjectionOn();

	m_ren->SetActiveCamera(m_cam);
	m_ren->ResetCamera();
	m_ren->InteractiveOff();
	m_ren->AddActor2D(m_textActor);
	m_ren->AddActor(m_bgImageActor);
	m_ren->AddActor(m_imageActor);

	m_renWnd->AddRenderer(m_ren);
	m_renWnd->DoubleBufferOn();

	m_qvtkWidget->SetRenderWindow(m_renWnd);
	m_qvtkWidget->SetCrossHair(false);
	m_qvtkWidget->SetFrameStyle(iAFramedQVTKWidget2::FRAMED);
	m_qvtkWidget->setEnabled(enabled);
	if (enabled)
		m_qvtkWidget->show();

	m_renWnd->GetInteractor()->Disable();
}

iAMagicLens::iAMagicLens() :
	m_isEnabled(false), 
	m_splitPosition(0.65f),
	m_isInitialized(false),
	m_size(DEFAULT_SIZE),
	m_maxLensCount(1),
	m_interpolate(false)
{
	m_viewMode = CENTERED;
}

void iAMagicLens::SetFrameWidth(qreal frameWidth)
{
	for (LensData & l : m_lenses)
	{
		l.m_qvtkWidget->SetFrameWidth(frameWidth);
		l.m_textActor->SetPosition(GetFrameWidth() + CaptionFrameDistance, GetFrameWidth() + CaptionFrameDistance);
	}
}

// width of the frame line
qreal iAMagicLens::GetFrameWidth() const
{
	if (m_lenses.size() > 0)
	{
		return m_lenses[0].m_qvtkWidget->GetFrameWidth();
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
		for (LensData & l : m_lenses)
		{
			l.m_renWnd->Render();
		}
}

// shows or hides the magic lense on the sliced image
void iAMagicLens::SetEnabled( bool isEnabled )
{
	m_isEnabled = isEnabled;
	for (LensData & l: m_lenses)
	{
		if (m_isEnabled)
			l.m_qvtkWidget->show();
		else
			l.m_qvtkWidget->hide();
	}
}

void iAMagicLens::SetGeometry( QRect & rect )
{
	m_viewedRect = rect;
	for (LensData & l : m_lenses)
	{
		QRect offsetRect = QRect(rect.x() + l.m_offset[0],
			rect.y() + l.m_offset[1],
			rect.width(), rect.height());

		if (m_viewMode == SIDE_BY_SIDE)
		{
			int splitOffset = GetSplitOffset();
			offsetRect = QRect(offsetRect.x() + splitOffset, offsetRect.y(),
				rect.width() - splitOffset, rect.height());
		}
		l.m_qvtkWidget->setGeometry(offsetRect);
	}
}

void iAMagicLens::InitWidget( QWidget * parent, const QGLWidget * shareWidget, Qt::WindowFlags f)
{
	m_parent = parent;
	m_shareWidget = shareWidget;
	m_flags = f;
}


void iAMagicLens::SetScaleCoefficient( double scaleCoefficient )
{
	m_scaleCoefficient = scaleCoefficient;
} 

void iAMagicLens::UpdateCamera( double focalPt[3], vtkCamera * cam )
{
	for (LensData & l : m_lenses)
	{
		//set focal point and corresponding scale
		l.m_cam->SetFocalPoint(focalPt);
		l.m_cam->SetParallelScale(cam->GetParallelScale()*m_scaleCoefficient);
		l.m_cam->SetRoll(cam->GetRoll());

		//set camera position
		double dir[3]; cam->GetDirectionOfProjection(dir);
		double res[3];
		vtkMath::Subtract(focalPt, dir, res);
		l.m_cam->SetPosition(res);
	}
}

void iAMagicLens::Repaint()
{
	for (LensData & l : m_lenses)
	{
		l.m_qvtkWidget->repaint();
	}
}

void iAMagicLens::Frame()
{
	for (LensData & l : m_lenses)
	{
		l.m_renWnd->Frame();
	}
}

bool iAMagicLens::Enabled()
{
	return m_isEnabled;
}

void iAMagicLens::SetViewMode( ViewMode mode )
{
	if (mode != OFFSET && m_maxLensCount > 1)
	{
		SetLensCount(1);
	}
	m_viewMode = mode;
	for (LensData & l : m_lenses)
	{
		if (l.m_qvtkWidget)
		{
			l.m_qvtkWidget->SetCrossHair(m_viewMode == OFFSET);
		}
	}
	UpdateOffset();
	UpdateShowFrame();
}

void iAMagicLens::UpdateOffset()
{
	for (int i = 0; i < m_lenses.size(); ++i)
	{
		switch (m_viewMode)
		{
		case CENTERED:
		case SIDE_BY_SIDE:
			m_lenses[i].m_offset[0] = m_lenses[i].m_offset[1] = 0;
			break;
		case OFFSET:
			switch (i)
			{	// current pattern: 8 neighbours around middle, starting at right middle
			case 0:	case 1:	case 7:
				m_lenses[i].m_offset[0] = m_size + OFFSET_MODE_X_OFFSET;
				break;
			case 2:	case 6:
				m_lenses[i].m_offset[0] = 0;
				break;
			case 3:	case 4:	case 5:
				m_lenses[i].m_offset[0] = - (m_size + OFFSET_MODE_X_OFFSET);
				break;
			}
			switch (i)
			{
			case 0:	case 4:
				m_lenses[i].m_offset[1] = 0;
				break;
			case 1:	case 2: case 3:
				m_lenses[i].m_offset[1] = -(m_size + OFFSET_MODE_X_OFFSET);
				break;
			case 5:	case 6:	case 7:
				m_lenses[i].m_offset[1] = m_size + OFFSET_MODE_X_OFFSET;
				break;
			}
			break;
		}
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
	for (LensData & l : m_lenses)
	{
		l.m_qvtkWidget->SetFrameStyle(frameStyle);
	}
}

int iAMagicLens::GetSplitOffset() const
{
	return qRound( m_viewedRect.width() * (1.f - m_splitPosition) );
}

int iAMagicLens::GetCenterSplitOffset() const
{
	return qRound( 0.5 * m_viewedRect.width() * (1.f - m_splitPosition) );
}

void iAMagicLens::UpdateLensInput(LensData & l, vtkImageReslice * reslicer, vtkScalarsToColors* cTF,
	vtkImageReslice * bgReslice, vtkScalarsToColors* bgCTF, QString const & name)
{
	l.m_imageToColors->SetInputConnection(reslicer->GetOutputPort());
	l.m_imageToColors->SetLookupTable(cTF);
	l.m_imageToColors->Update();
	l.m_bgImageToColors->SetInputConnection(bgReslice->GetOutputPort());
	l.m_bgImageToColors->SetLookupTable(bgCTF);
	l.m_bgImageToColors->Update();
	l.m_textActor->SetInput(name.toStdString().c_str());
}

void iAMagicLens::AddInput(
	vtkImageReslice * reslicer,  vtkScalarsToColors* cTF,
	vtkImageReslice * bgReslice, vtkScalarsToColors* bgCTF,
	QString const & name)
{
	if (!reslicer->GetInput())
	{
		return;
	}
	if (m_lenses.size() != 1 || m_maxLensCount > 1)
	{
		if (m_lenses.size() == m_maxLensCount)
		{
			m_lenses[0].m_qvtkWidget->hide();
			delete m_lenses[0].m_qvtkWidget;
			m_lenses.remove(0);
		}
		LensData l(m_parent, m_shareWidget, m_flags, m_interpolate, m_isEnabled);
		l.m_qvtkWidget->SetCrossHair(m_viewMode == OFFSET);
		m_lenses.append(l);
		UpdateLensInput(l, reslicer, cTF, bgReslice, bgCTF, name);
	}
	else
	{
		UpdateLensInput(m_lenses[0], reslicer, cTF, bgReslice, bgCTF, name);
	}
	m_isInitialized = true;
	UpdateOffset();
	UpdateShowFrame();
}


void iAMagicLens::SetLensCount(int count)
{
	m_maxLensCount = count;
	while (count < m_lenses.size())
	{
		delete m_lenses[0].m_qvtkWidget;
		m_lenses.remove(0);
	}
	m_lenses.reserve(count);
	if (count > 1 && m_viewMode != OFFSET)
	{
		SetViewMode(OFFSET);
	}
}


void iAMagicLens::UpdateColors()
{
	if (m_isInitialized)
	{
		for (LensData & l : m_lenses)
		{
			l.m_imageToColors->Update();
			l.m_bgImageToColors->Update();
		}
	}
}


int iAMagicLens::GetSize() const
{
	return m_size;
}


void iAMagicLens::SetSize(int newSize)
{
	if (newSize < MinimumMagicLensSize || newSize > MaximumMagicLensSize)
	{
		return;
	}
	m_size = newSize;
	UpdateOffset();
}

int iAMagicLens::GetOffset() const
{
	return m_size + OFFSET_MODE_X_OFFSET;
}

void iAMagicLens::SetInterpolate(bool on)
{
	m_interpolate = on;
	for (LensData & l : m_lenses)
	{
		l.m_imageActor->SetInterpolate(on);
		l.m_bgImageActor->SetInterpolate(on);
	}
}

void iAMagicLens::SetOpacity(double opacity)
{
	for (LensData & l : m_lenses)
	{
		l.m_imageActor->SetOpacity(opacity);
	}
}

double iAMagicLens::GetOpacity()
{
	return (m_lenses.size() == 0) ? 0 : m_lenses[0].m_imageActor->GetOpacity();
}

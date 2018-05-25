/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAMagicLens.h"

#include "defines.h"
#include "iAConsole.h"

#include <QVTKInteractor.h>
#include <QVTKInteractorAdapter.h>
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarsToColors.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QSharedPointer>

const int iAMagicLens::DefaultSize = 120;
const double iAMagicLens::DefaultFrameWidth = 5;
const int iAMagicLens::OffsetModeXOffset = 10;

namespace
{
	const int CaptionFrameDistance = 0;
	const int CaptionFontSize = 13;

	void CalculateViewPort(double viewPort[4], int const winSize[2], int const mousePos[2], int size, int const offset[2])
	{
		double halfSize = size / 2.0;
		for (int i = 0; i < 2; ++i)
		{
			viewPort[0+i] = (mousePos[i] - halfSize + offset[i]) / winSize[i];
			viewPort[2+i] = (mousePos[i] + halfSize + offset[i]) / winSize[i];
		}
	}
}


class iALensData
{
public:
	iALensData(vtkGenericOpenGLRenderWindow* renderWindow, double opacity, int size, double frameWidth, bool interpolate, bool enabled);
	void SetLensVisible(bool visible);
	void SetCrossHairVisible(bool enable);
	void SetFrameWidth(double frameWidth);
	void SetOffset(int xofs, int yofs);
	void SetInterpolate(bool interpolate);
	void SetOpacity(double opacity);
	void SetSize(int size);
	void UpdateContent(vtkImageReslice * reslicer, vtkScalarsToColors* cTF, QString const & name);
	void UpdatePosition(double const focalPt[3], double const * dir, double parallelScale, int const mousePos[2]);
	void UpdateColors();
	void Render();
private:
	void UpdateViewPort(int const mousePos[2]);

	vtkSmartPointer<vtkImageMapToColors> m_imageColors;
	vtkSmartPointer<vtkImageActor> m_imageActor;
	vtkSmartPointer<vtkRenderer> m_imageRenderer;
	//vtkSmartPointer<vtkPolyData> m_frameData;
	//vtkSmartPointer<vtkPolyDataMapper2D> m_frameMapper;
	//vtkSmartPointer<vtkActor2D> m_frameActor;
	//vtkSmartPointer<vtkPolyData> m_crossHairData;
	//vtkSmartPointer<vtkPolyDataMapper2D> m_crossHairMapper;
	//vtkSmartPointer<vtkActor2D> m_crossHairActor;
	vtkSmartPointer<vtkTextActor> m_textActor;
	vtkSmartPointer<vtkRenderer> m_guiRenderer;
	vtkGenericOpenGLRenderWindow* m_renderWindow;
	int m_offset[2];
	int m_size;
	//QRect m_viewedRect;	// rect of the area of data actually displayed using m-lens
	bool m_showCrosshair;
};


iALensData::iALensData(vtkGenericOpenGLRenderWindow* renderWindow, double opacity, int size, double frameWidth, bool interpolate, bool enabled) :
	m_imageColors(vtkSmartPointer<vtkImageMapToColors>::New()),
	m_imageActor(vtkSmartPointer<vtkImageActor>::New()),
	m_imageRenderer(vtkSmartPointer<vtkRenderer>::New()),
	//m_frameData(vtkSmartPointer<vtkPolyData>::New()),
	//m_frameMapper(vtkSmartPointer<vtkPolyDataMapper2D>::New()),
	//m_frameActor(vtkSmartPointer<vtkActor2D>::New()),
	//m_crossHairData(vtkSmartPointer<vtkPolyData>::New()),
	//m_crossHairMapper(vtkSmartPointer<vtkPolyDataMapper2D>()),
	//m_crossHairActor(vtkSmartPointer<vtkActor2D>::New()),
	m_textActor(vtkSmartPointer<vtkTextActor>::New()),
	m_guiRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_renderWindow(renderWindow),
	m_size(size),
	m_offset{0, 0}
{
	m_imageActor->SetInputData(m_imageColors->GetOutput());
	m_imageActor->GetMapper()->BorderOn();
	m_imageActor->SetOpacity(opacity);
	SetInterpolate(interpolate);
/*
	// ORIENTATION / ROTATION FIX:
	double orientation[3] = {180, 0, 0};
	m_imageActor->SetOrientation(orientation);
*/
	m_imageRenderer->InteractiveOff();
	m_imageRenderer->AddActor(m_imageActor);
	m_imageRenderer->SetLayer(1);
	m_imageRenderer->GetActiveCamera()->ParallelProjectionOn();

	m_textActor->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
	m_textActor->GetTextProperty()->SetBackgroundColor(1.0, 1.0, 1.0);
	m_textActor->GetTextProperty()->SetBackgroundOpacity(0.5);
	m_textActor->GetTextProperty()->SetFontSize(CaptionFontSize);
	double textMargin = frameWidth + CaptionFrameDistance;
	m_textActor->SetPosition(textMargin, textMargin);
	
	m_guiRenderer->SetLayer(2);
	//m_guiRenderer->AddActor2D(m_frameActor);
	m_guiRenderer->AddActor2D(m_textActor);

	if (enabled)
		SetLensVisible(enabled);
}

void iALensData::SetLensVisible(bool enabled)
{
	if (!m_renderWindow)
	{
		DEBUG_LOG("ERROR in Magic Lens: No render window set!");
		return;
	}
	if (enabled)
	{
		m_renderWindow->AddRenderer(m_imageRenderer);
		m_renderWindow->AddRenderer(m_guiRenderer);
	}
	else
	{
		m_renderWindow->RemoveRenderer(m_imageRenderer);
		m_renderWindow->RemoveRenderer(m_guiRenderer);
	}
}

void iALensData::SetCrossHairVisible(bool enabled)
{
	/*
	if (enabled)
		m_guiRenderer->AddActor(m_crossHairActor);
	else
		m_guiRenderer->RemoveActor(m_crossHairActor);
	*/
}

void iALensData::SetFrameWidth(double frameWidth)
{
	//m_frameActor->GetProperty()->SetLineWidth(frameWidth);
	m_textActor->SetPosition(frameWidth + CaptionFrameDistance, frameWidth + CaptionFrameDistance);
}

void iALensData::UpdateViewPort(int const mousePos[2])
{
	double viewPort[4];
	int const * windowSize = m_renderWindow->GetSize();
	CalculateViewPort(viewPort, windowSize, mousePos, m_size, m_offset);
	DEBUG_LOG(QString("  Mouse position: (%1, %2), size=%3, window size=(%4, %5), offset=(%6, %7)")
		.arg(mousePos[0]).arg(mousePos[1]).arg(m_size)
		.arg(windowSize[0]).arg(windowSize[1])
		.arg(m_offset[0]).arg(m_offset[1]));
	DEBUG_LOG(QString("  Viewport borders = (%1, %2, %3, %4)")
		.arg(viewPort[0]).arg(viewPort[1]).arg(viewPort[2]).arg(viewPort[3]));
	m_imageRenderer->SetViewport(viewPort);
	m_guiRenderer->SetViewport(viewPort);
}

void iALensData::SetOffset(int xofs, int yofs)
{
	m_offset[0] = xofs;
	m_offset[1] = yofs;
}

void iALensData::SetInterpolate(bool interpolate)
{
	m_imageActor->SetInterpolate(interpolate);
}

void iALensData::SetOpacity(double opacity)
{
	m_imageActor->SetOpacity(opacity);
}

void iALensData::SetSize(int size)
{
	m_size = size;
	// TODO: updates?
}

void iALensData::UpdateContent(vtkImageReslice * reslicer, vtkScalarsToColors* cTF, QString const & name)
{
	m_imageColors->SetInputConnection(reslicer->GetOutputPort());
	m_imageColors->SetLookupTable(cTF);
	m_imageColors->Update();
	m_textActor->SetInput(name.toStdString().c_str());
}

void iALensData::UpdatePosition(double const focalPt[3], double const * dir, double parallelScale, int const mousePos[2])
{
	m_imageRenderer->GetActiveCamera()->SetFocalPoint(focalPt);
	m_imageRenderer->GetActiveCamera()->SetParallelScale(parallelScale);
	double camPos[3];
	vtkMath::Subtract(focalPt, dir, camPos);
	m_imageRenderer->GetActiveCamera()->SetPosition(camPos);
	DEBUG_LOG(QString("  Camera: focal point=(%1, %2, %3), parallelScale=%4, position=(%5, %6, %7), direction=(%8, %9, %10)")
		.arg(focalPt[0]).arg(focalPt[1]).arg(focalPt[2])
		.arg(parallelScale)
		.arg(camPos[0]).arg(camPos[1]).arg(camPos[2])
		.arg(dir[0]).arg(dir[1]).arg(dir[2]));
	UpdateViewPort(mousePos);
}

void iALensData::UpdateColors()
{
	m_imageColors->Update();
}

void iALensData::Render()
{
	if (!m_renderWindow)
		return;
	m_imageRenderer->Render();
	m_guiRenderer->Render();
}


// iAMagicLens

iAMagicLens::iAMagicLens() :
	m_isEnabled(false),
	m_isInitialized(false),
	m_size(DefaultSize),
	m_frameWidth(DefaultFrameWidth),
	m_maxLensCount(1),
	m_interpolate(false),
	m_viewMode(CENTERED),
	m_opacity(1.0),
	m_srcWindowRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_renderWindow(nullptr)
{}

void iAMagicLens::SetFrameWidth(qreal frameWidth)
{
	m_frameWidth = frameWidth;
	for (auto l : m_lenses)
		l->SetFrameWidth(frameWidth);
}

qreal iAMagicLens::GetFrameWidth() const
{
	return m_frameWidth;
}

void iAMagicLens::SetEnabled( bool isEnabled )
{
	m_isEnabled = isEnabled;
	for (auto l: m_lenses)
		l->SetLensVisible(m_isEnabled);
	if (m_viewMode == OFFSET)
		SetSrcWindowEnabled(isEnabled);
}

void iAMagicLens::SetRenderWindow(vtkGenericOpenGLRenderWindow* renderWindow)
{
	m_renderWindow = renderWindow;
}

void iAMagicLens::UpdatePosition(vtkCamera * cam, double const lensPos[3], int const mousePos[2])
{
	int const * windowSize = m_renderWindow->GetSize();
	double scaleCoefficient = (windowSize[1] == 0) ? 1 : static_cast<double>(m_size) / windowSize[1];
	for (auto l : m_lenses)
		l->UpdatePosition(lensPos, cam->GetDirectionOfProjection(), cam->GetParallelScale()*scaleCoefficient, mousePos);
	double viewPort[4];
	int offset[2] = { 0, 0 };
	CalculateViewPort(viewPort, m_renderWindow->GetSize(), mousePos, m_size, offset);
	m_srcWindowRenderer->SetViewport(viewPort);
}

bool iAMagicLens::IsEnabled()
{
	return m_isEnabled;
}

void iAMagicLens::SetViewMode( ViewMode mode )
{
	if (mode != OFFSET && m_maxLensCount > 1)
		SetLensCount(1);
	m_viewMode = mode;
	for (auto l : m_lenses)
		l->SetCrossHairVisible(m_viewMode == OFFSET);
	SetSrcWindowEnabled(m_viewMode == OFFSET);
	UpdateOffset();
}

void iAMagicLens::SetSrcWindowEnabled(bool enabled)
{
	if (enabled)
		m_renderWindow->AddRenderer(m_srcWindowRenderer);
	else
		m_renderWindow->RemoveRenderer(m_srcWindowRenderer);
}

void iAMagicLens::UpdateOffset()
{
	for (int i = 0; i < m_lenses.size(); ++i)
	{
		int xofs = 0, yofs = 0;
		switch (m_viewMode)
		{
		default:
		case CENTERED:  break; // xofs and yofs = 00
		case OFFSET:
			switch (i)
			{	// current pattern: 8 neighbours around middle, starting at right middle
			case 0:	case 1:	case 7: xofs = m_size + OffsetModeXOffset;	   break;
			case 2:	case 6: break;  // xofs = 0
			case 3:	case 4:	case 5: xofs = - (m_size + OffsetModeXOffset); break;
			}
			switch (i)
			{
			case 0:	case 4: break; // yofs = 0
			case 1:	case 2: case 3: yofs = -(m_size + OffsetModeXOffset);  break;
			case 5:	case 6:	case 7: yofs = m_size + OffsetModeXOffset;     break;
			}
			break;
		}
		m_lenses[i]->SetOffset(xofs, yofs);
	}
}

iAMagicLens::ViewMode iAMagicLens::GetViewMode() const
{
	return m_viewMode;
}

void iAMagicLens::AddInput(vtkImageReslice * reslicer,  vtkScalarsToColors* cTF, QString const & name)
{
	if (!reslicer->GetInput())
	{
		return;
	}
	if (m_lenses.size() != 1 || m_maxLensCount > 1)
	{
		if (m_lenses.size() == m_maxLensCount)
		{
			m_lenses[0]->SetLensVisible(false);
			m_lenses.remove(0);
		}
		QSharedPointer<iALensData> l(new iALensData(m_renderWindow, m_opacity, m_size, m_frameWidth, m_interpolate, m_isEnabled));
		l->SetCrossHairVisible(m_viewMode == OFFSET);
		m_lenses.append(l);
		l->UpdateContent(reslicer, cTF, name);
	}
	else
	{
		m_lenses[0]->UpdateContent(reslicer, cTF, name);
	}
	m_isInitialized = true;
	UpdateOffset();
}

void iAMagicLens::SetLensCount(int count)
{
	m_maxLensCount = count;
	while (count < m_lenses.size())
	{
		m_lenses[0]->SetLensVisible(false);
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
		for (auto l : m_lenses)
			l->UpdateColors();
}

int iAMagicLens::GetSize() const
{
	return m_size;
}

void iAMagicLens::SetSize(int newSize)
{
	if (newSize < MinimumMagicLensSize || newSize > MaximumMagicLensSize)
		return;
	m_size = newSize;
	for (auto l : m_lenses)
		l->SetSize(newSize);
	UpdateOffset();
}

void iAMagicLens::SetInterpolate(bool on)
{
	m_interpolate = on;
	for (auto l : m_lenses)
		l->SetInterpolate(on);
}

void iAMagicLens::SetOpacity(double opacity)
{
	m_opacity = opacity;
	for (auto l : m_lenses)
		l->SetOpacity(opacity);
}

double iAMagicLens::GetOpacity() const
{
	return m_opacity;
}

void iAMagicLens::Render()
{
	if (!m_isEnabled || !m_renderWindow)
		return;
	if (m_viewMode == OFFSET)
		m_srcWindowRenderer->Render();
	for (auto l : m_lenses)
		l->Render();
}

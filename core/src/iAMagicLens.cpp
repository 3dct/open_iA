/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAMagicLens.h"

#include "defines.h"
#include "iAConsole.h"
#include "iAMathUtility.h"

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
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
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


// rewrite utilizing iAChannelSlicerData - that has a vtkImageMapToColors and a vtkImageActor already!
//! Information about a single lens window of an iAMagicLens.
class iALensData
{
public:
	iALensData(vtkGenericOpenGLRenderWindow* renderWindow, double opacity, int size, double frameWidth, bool interpolate, bool enabled);
	void setLensVisible(bool visible);
	void setFrameWidth(double frameWidth);
	void setOffset(int xofs, int yofs);
	void setInterpolate(bool interpolate);
	void setOpacity(double opacity);
	void setSize(int size);
	void updateContent(vtkImageReslice * reslicer, vtkScalarsToColors* cTF, QString const & name);
	void updatePosition(double const focalPt[3], double const * dir, double parallelScale, int const mousePos[2]);
	void updateColors();
	void render();
private:
	void updateViewPort(int const mousePos[2]);

	vtkSmartPointer<vtkImageMapToColors> m_imageColors;
	vtkSmartPointer<vtkImageActor> m_imageActor;
	vtkSmartPointer<vtkRenderer> m_imageRenderer;
	vtkSmartPointer<vtkPolyData> m_frameData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_frameMapper;
	vtkSmartPointer<vtkActor2D> m_frameActor;
	vtkSmartPointer<vtkTextActor> m_textActor;
	vtkSmartPointer<vtkRenderer> m_guiRenderer;
	vtkGenericOpenGLRenderWindow* m_renderWindow;
	int m_offset[2];
	int m_size;
	double m_frameWidth;
};


iALensData::iALensData(vtkGenericOpenGLRenderWindow* renderWindow, double opacity, int size, double frameWidth, bool interpolate, bool enabled) :
	m_imageColors(vtkSmartPointer<vtkImageMapToColors>::New()),
	m_imageActor(vtkSmartPointer<vtkImageActor>::New()),
	m_imageRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_frameData(vtkSmartPointer<vtkPolyData>::New()),
	m_frameMapper(vtkSmartPointer<vtkPolyDataMapper2D>::New()),
	m_frameActor(vtkSmartPointer<vtkActor2D>::New()),
	m_textActor(vtkSmartPointer<vtkTextActor>::New()),
	m_guiRenderer(vtkSmartPointer<vtkRenderer>::New()),
	m_renderWindow(renderWindow),
	m_size(size),
	m_frameWidth(frameWidth)
{
	m_offset[0] = m_offset[1] = 0;
	m_imageActor->SetInputData(m_imageColors->GetOutput());
	m_imageActor->GetMapper()->BorderOn();
	m_imageActor->SetOpacity(opacity);
	setInterpolate(interpolate);
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

	m_frameMapper->SetInputData(m_frameData);
	m_frameActor->GetProperty()->SetColor(1., 1., 0);
	setFrameWidth(frameWidth);
	m_frameActor->SetMapper(m_frameMapper);

	m_guiRenderer->SetLayer(2);
	m_guiRenderer->AddActor2D(m_frameActor);
	m_guiRenderer->AddActor2D(m_textActor);

	if (enabled)
		setLensVisible(enabled);
}

void iALensData::setLensVisible(bool enabled)
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

void iALensData::setFrameWidth(double frameWidth)
{
	m_frameWidth = frameWidth;
	m_frameActor->GetProperty()->SetLineWidth(frameWidth);
	m_textActor->SetPosition(frameWidth + CaptionFrameDistance, frameWidth + CaptionFrameDistance);
}

void iALensData::updateViewPort(int const mousePos[2])
{
	double viewPort[4];
	int const * windowSize = m_renderWindow->GetSize();
	CalculateViewPort(viewPort, windowSize, mousePos, m_size, m_offset);
	m_imageRenderer->SetViewport(viewPort);
	m_guiRenderer->SetViewport(viewPort);

	// update border:
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkPolyLine> line = vtkSmartPointer<vtkPolyLine>::New();
	double fwHalf = m_frameWidth / 2;
	double p0[3] = { fwHalf, fwHalf, 0.0 };
	double p1[3] = { fwHalf, static_cast<double>(m_size)-fwHalf, 0.0 };
	double p2[3] = { static_cast<double>(m_size)-fwHalf, static_cast<double>(m_size)-fwHalf, 0.0 };
	double p3[3] = { static_cast<double>(m_size)-fwHalf, fwHalf, 0.0 };
	points->InsertNextPoint(p0);
	points->InsertNextPoint(p1);
	points->InsertNextPoint(p2);
	points->InsertNextPoint(p3);
	line->GetPointIds()->SetNumberOfIds(5);
	for (int i = 0; i < 5; i++)
		line->GetPointIds()->SetId(i, i % 4);
	cells->InsertNextCell(line);

	m_frameData->SetPoints(points);
	m_frameData->SetLines(cells);
	m_frameMapper->Update();
}

void iALensData::setOffset(int xofs, int yofs)
{
	m_offset[0] = xofs;
	m_offset[1] = yofs;
}

void iALensData::setInterpolate(bool interpolate)
{
	m_imageActor->SetInterpolate(interpolate);
}

void iALensData::setOpacity(double opacity)
{
	m_imageActor->SetOpacity(opacity);
}

void iALensData::setSize(int size)
{
	m_size = size;
}

void iALensData::updateContent(vtkImageReslice * reslicer, vtkScalarsToColors* cTF, QString const & name)
{
	m_imageColors->SetInputConnection(reslicer->GetOutputPort());
	m_imageColors->SetLookupTable(cTF);
	m_imageColors->Update();
	m_textActor->SetInput(name.toStdString().c_str());
}

void iALensData::updatePosition(double const focalPt[3], double const * dir, double parallelScale, int const mousePos[2])
{
	m_imageRenderer->GetActiveCamera()->SetFocalPoint(focalPt);
	m_imageRenderer->GetActiveCamera()->SetParallelScale(parallelScale);
	double camPos[3];
	vtkMath::Subtract(focalPt, dir, camPos);
	m_imageRenderer->GetActiveCamera()->SetPosition(camPos);
	updateViewPort(mousePos);
}

void iALensData::updateColors()
{
	m_imageColors->Update();
}

void iALensData::render()
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
	m_maxLensCount(1),
	m_size(DefaultMagicLensSize),
	m_frameWidth(DefaultFrameWidth),
	m_interpolate(false),
	m_viewMode(CENTERED),
	m_opacity(1.0),
	m_renderWindow(nullptr),
	m_srcWindowData(vtkSmartPointer<vtkPolyData>::New()),
	m_srcWindowMapper(vtkSmartPointer<vtkPolyDataMapper2D>::New()),
	m_srcWindowActor(vtkSmartPointer<vtkActor2D>::New()),
	m_srcWindowRenderer(vtkSmartPointer<vtkRenderer>::New())
{
	m_srcWindowMapper->SetInputData(m_srcWindowData);
	m_srcWindowActor->GetProperty()->SetColor(1., 1., 1.);
	m_srcWindowActor->GetProperty()->SetLineWidth(m_frameWidth);
	m_srcWindowActor->SetMapper(m_srcWindowMapper);
	m_srcWindowRenderer->SetLayer(1);
	m_srcWindowRenderer->AddActor(m_srcWindowActor);
}

void iAMagicLens::setFrameWidth(qreal frameWidth)
{
	m_frameWidth = frameWidth;
	m_srcWindowActor->GetProperty()->SetLineWidth(m_frameWidth);
	for (auto l : m_lenses)
		l->setFrameWidth(frameWidth);
}

qreal iAMagicLens::frameWidth() const
{
	return m_frameWidth;
}

void iAMagicLens::setEnabled( bool isEnabled )
{
	m_isEnabled = isEnabled;
	for (auto l: m_lenses)
		l->setLensVisible(m_isEnabled);
	if (m_viewMode == OFFSET)
		setSrcWindowEnabled(isEnabled);
}

void iAMagicLens::setRenderWindow(vtkGenericOpenGLRenderWindow* renderWindow)
{
	m_renderWindow = renderWindow;
}

void iAMagicLens::updatePosition(vtkCamera * cam, double const lensPos[3], int const mousePos[2])
{
	int const * windowSize = m_renderWindow->GetSize();
	double scaleCoefficient = (windowSize[1] == 0) ? 1 : static_cast<double>(m_size) / windowSize[1];
	for (auto l : m_lenses)
		l->updatePosition(lensPos, cam->GetDirectionOfProjection(), cam->GetParallelScale()*scaleCoefficient, mousePos);

	if (m_viewMode == OFFSET)
	{
		double viewPort[4];
		int offset[2] = { 0, 0 };
		CalculateViewPort(viewPort, m_renderWindow->GetSize(), mousePos, m_size, offset);
		m_srcWindowRenderer->SetViewport(viewPort);
		double fwHalf = frameWidth() / 2;
		// update border:
		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
		vtkSmartPointer<vtkPolyLine> line = vtkSmartPointer<vtkPolyLine>::New();
		double p0[3] = { fwHalf, fwHalf, 0.0 };
		double p1[3] = { fwHalf, static_cast<double>(m_size)-fwHalf, 0.0 };
		double p2[3] = { static_cast<double>(m_size)-fwHalf, static_cast<double>(m_size)-fwHalf, 0.0 };
		double p3[3] = { static_cast<double>(m_size)-fwHalf, fwHalf, 0.0 };
		points->InsertNextPoint(p0);
		points->InsertNextPoint(p1);
		points->InsertNextPoint(p2);
		points->InsertNextPoint(p3);
		line->GetPointIds()->SetNumberOfIds(5);
		for (int i = 0; i < 5; i++)
			line->GetPointIds()->SetId(i, i % 4);
		cells->InsertNextCell(line);
		m_srcWindowData->SetPoints(points);
		m_srcWindowData->SetLines(cells);
		m_srcWindowMapper->Update();
	}
}

bool iAMagicLens::isEnabled()
{
	return m_isEnabled;
}

void iAMagicLens::setViewMode( ViewMode mode )
{
	if (mode != OFFSET && m_maxLensCount > 1)
		setLensCount(1);
	m_viewMode = mode;
	for (auto l : m_lenses)
		l->setOpacity(m_viewMode == OFFSET ? 1.0 : m_opacity);
	setSrcWindowEnabled(m_viewMode == OFFSET);
	updateOffset();
}

void iAMagicLens::setSrcWindowEnabled(bool enabled)
{
	if (enabled)
		m_renderWindow->AddRenderer(m_srcWindowRenderer);
	else
		m_renderWindow->RemoveRenderer(m_srcWindowRenderer);
}

void iAMagicLens::updateOffset()
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
		m_lenses[i]->setOffset(xofs, yofs);
	}
}

iAMagicLens::ViewMode iAMagicLens::viewMode() const
{
	return m_viewMode;
}

void iAMagicLens::addInput(vtkImageReslice * reslicer,  vtkScalarsToColors* cTF, QString const & name)
{
	if (!reslicer->GetInput())
	{
		return;
	}
	if (m_lenses.size() != 1 || m_maxLensCount > 1)
	{
		if (m_lenses.size() == m_maxLensCount)
		{
			m_lenses[0]->setLensVisible(false);
			m_lenses.remove(0);
		}
		QSharedPointer<iALensData> l(new iALensData(m_renderWindow, m_opacity, m_size, m_frameWidth, m_interpolate, m_isEnabled));
		m_lenses.append(l);
		l->updateContent(reslicer, cTF, name);
	}
	else
	{
		m_lenses[0]->updateContent(reslicer, cTF, name);
	}
	m_isInitialized = true;
	updateOffset();
}

void iAMagicLens::setLensCount(int count)
{
	m_maxLensCount = count;
	while (count < m_lenses.size())
	{
		m_lenses[0]->setLensVisible(false);
		m_lenses.remove(0);
	}
	m_lenses.reserve(count);
	if (count > 1 && m_viewMode != OFFSET)
	{
		setViewMode(OFFSET);
	}
}

void iAMagicLens::updateColors()
{
	if (m_isInitialized)
		for (auto l : m_lenses)
			l->updateColors();
}

int iAMagicLens::size() const
{
	return m_size;
}

void iAMagicLens::setSize(int newSize)
{
	int maxSize = MaximumMagicLensSize;
	if (m_renderWindow)
	{
		int const * windowSize = m_renderWindow->GetSize();
		int maxDim = std::min(windowSize[0], windowSize[1]);
		if (maxDim > 0)
			maxSize = std::min(MaximumMagicLensSize, maxDim);
	}
	newSize = clamp(MinimumMagicLensSize, maxSize, newSize);
	if (m_size == newSize)
		return;
	m_size = newSize;
	for (auto l : m_lenses)
		l->setSize(newSize);
	updateOffset();
}

void iAMagicLens::setInterpolate(bool on)
{
	m_interpolate = on;
	for (auto l : m_lenses)
		l->setInterpolate(on);
}

void iAMagicLens::setOpacity(double opacity)
{
	if (m_viewMode == OFFSET)
		return;
	m_opacity = opacity;
	for (auto l : m_lenses)
		l->setOpacity(opacity);
}

double iAMagicLens::opacity() const
{
	return m_opacity;
}

void iAMagicLens::render()
{
	if (!m_isEnabled || !m_renderWindow)
		return;
	if (m_viewMode == OFFSET)
		m_srcWindowRenderer->Render();
	for (auto l : m_lenses)
		l->render();
}

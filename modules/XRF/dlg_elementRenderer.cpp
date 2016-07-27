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
#include "dlg_elementRenderer.h"

#include "iARenderer.h"
#include "iATransferFunction.h"
#include "iAVolumeRenderer.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyData.h>
#include <vtkTransform.h>

dlg_elementRenderer::dlg_elementRenderer(QWidget *parent):
	dlg_elemRendererContainer(parent),
	m_renderer( new iARenderer(this) ),
	m_rendInitialized(false),
	m_axesTransform( vtkSmartPointer<vtkTransform>::New() ),
	m_observedRenderer(0),
	m_tag(0),
	m_indexInReferenceLib(-1)
{
	renContainer->SetRenderWindow(m_renderer->GetRenderWindow());
	m_renderer->GetRenderer()->InteractiveOff();
	m_renderer->setAxesTransform(m_axesTransform);

	connect(renContainer, SIGNAL(rightButtonReleasedSignal()), m_renderer, SLOT(mouseRightButtonReleasedSlot()) );
	connect(renContainer, SIGNAL(leftButtonReleasedSignal()), m_renderer, SLOT(mouseLeftButtonReleasedSlot()) );
}


void dlg_elementRenderer::removeObserver()

{
	//is m_renderer deleted by Qt?
	if(m_observedRenderer)
		m_observedRenderer->RemoveObserver(m_tag);
}

void dlg_elementRenderer::SetDataToVisualize( vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	// TODO: VOLUME: check if working!
	iASimpleTransferFunction transferFunction(ctf, otf);
	if(!m_rendInitialized)
	{
		m_renderer->initialize(imgData, polyData);
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&transferFunction, imgData));
		m_volumeRenderer->AddToWindow(m_renderer->GetRenderWindow());
		m_rendInitialized = true;
	}
	else
	{
		// TODO: VOLUME: check if recreation of volume renderer is necessary!
		m_volumeRenderer->RemoveFromWindow();
		m_renderer->reInitialize(imgData, polyData);
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&transferFunction, imgData));
		m_volumeRenderer->AddToWindow(m_renderer->GetRenderWindow());
	}
}

iARenderer * dlg_elementRenderer::GetRenderer()
{
	return m_renderer;
}

void dlg_elementRenderer::SetRefLibIndex( size_t index )
{
	m_indexInReferenceLib = index;
}

size_t dlg_elementRenderer::GetRefLibIndex()
{
	return m_indexInReferenceLib;
}

void dlg_elementRenderer::ApplyVolumeSettings(iAVolumeSettings const & vs)
{
	m_volumeRenderer->ApplySettings(vs);
}

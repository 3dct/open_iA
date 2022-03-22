/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_elementRenderer.h"

#include <iARendererImpl.h>
#include <iATransferFunction.h>
#include <iAVolumeRenderer.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyData.h>
#include <vtkRendererCollection.h>
#include <vtkTransform.h>

dlg_elementRenderer::dlg_elementRenderer(QWidget *parent):
	dlg_elemRendererContainer(parent),
	m_renderer( new iARendererImpl(this) ),
	m_rendInitialized(false),
	m_axesTransform( vtkSmartPointer<vtkTransform>::New() ),
	m_indexInReferenceLib(std::numeric_limits<size_t>::max())
{
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	renContainer->SetRenderWindow(dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_renderer->renderWindow()));
#else
	renContainer->setRenderWindow(dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_renderer->renderWindow()));
#endif
	m_renderer->setAxesTransform(m_axesTransform);
	m_renderer->showHelpers(false);

	connect(renContainer, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	connect(renContainer, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
}

void dlg_elementRenderer::SetDataToVisualize( vtkImageData * imgData, vtkPolyData * polyData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	iASimpleTransferFunction transferFunction(ctf, otf);
	if(!m_rendInitialized)
	{
		m_renderer->initialize(imgData, polyData);
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>::create(&transferFunction, imgData);
		m_volumeRenderer->addTo(m_renderer->renderer());
		m_rendInitialized = true;
	}
	else
	{
		m_volumeRenderer->remove();
		m_renderer->reInitialize(imgData, polyData);
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>::create(&transferFunction, imgData);
		m_volumeRenderer->addTo(m_renderer->renderer());
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
	m_volumeRenderer->applySettings(vs);
}

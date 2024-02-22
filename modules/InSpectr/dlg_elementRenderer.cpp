// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_elementRenderer.h"

#include <iARendererImpl.h>
#include <iATransferFunctionPtrs.h>
#include <iAVolumeRenderer.h>

#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPiecewiseFunction.h>

dlg_elementRenderer::dlg_elementRenderer(QWidget *parent):
	dlg_elemRendererContainer(parent),
	m_renderer( new iARendererImpl(this, dynamic_cast<vtkGenericOpenGLRenderWindow*>(renContainer->renderWindow()) )),
	m_indexInReferenceLib(std::numeric_limits<size_t>::max())
{
	m_renderer->showAxesCube(false);
	m_renderer->showOriginIndicator(false);
	connect(renContainer, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	connect(renContainer, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
}

void dlg_elementRenderer::SetDataToVisualize( vtkImageData * imgData, vtkPiecewiseFunction* otf, vtkColorTransferFunction* ctf )
{
	m_transferFunction = std::make_shared<iATransferFunctionPtrs>(ctf, otf);
	m_volumeRenderer = std::make_shared<iAVolumeRenderer>(m_renderer->renderer(), imgData, m_transferFunction.get());
	m_volumeRenderer->setVisible(true);
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

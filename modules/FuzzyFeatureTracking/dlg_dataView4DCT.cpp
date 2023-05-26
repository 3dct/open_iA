// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_dataView4DCT.h"

#include "iAFuzzyVTKWidget.h"

#include <iAMdiChild.h>
#include <iARendererImpl.h>
#include <iATransferFunctionPtrs.h>
#include <iAVolumeRenderer.h>
#include <iAVolumeViewer.h>

#include <iADataSet.h>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkTransform.h>

#include <QHBoxLayout>

namespace
{
	const QString RendererBackground("#FFFFFF");
	const bool ShowHelpers = false;
}

dlg_dataView4DCT::dlg_dataView4DCT(QWidget* parent, std::vector<iAVolumeViewer*> const & volumeViewers):
	QWidget(parent),
	m_volumeViewers(volumeViewers),
	m_axesTransform(vtkSmartPointer<vtkTransform>::New())
{
	m_mdiChild = dynamic_cast<iAMdiChild*>(parent);

	m_rendererManager.addToBundle(m_mdiChild->renderer()->renderer());

	// add widgets to window
	size_t numOfVolumes = m_volumeViewers.size();
	m_vtkWidgets = new iAFuzzyVTKWidget*[numOfVolumes];
	m_renderers = new iARendererImpl*[numOfVolumes];
	m_volumeRenderer = new iAVolumeRenderer*[numOfVolumes];
	for(size_t i = 0; i < numOfVolumes; i++)
	{
		m_vtkWidgets[i] = new iAFuzzyVTKWidget(this);
		m_renderers[i] = new iARendererImpl(this, dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_vtkWidgets[i]->renderWindow()));
		m_volumeRenderer[i] = new iAVolumeRenderer(m_renderers[i]->renderer(), m_volumeViewers[i]->volume()->vtkImage(), m_volumeViewers[i]->transfer());
		m_renderers[i]->setAxesTransform(m_axesTransform);
		// TODO NEWIO: get volume stack and polydata dataset in here...
		//m_renderers[i]->initialize(m_volumeStack->volume(i), m_mdiChild->polyData());
		m_volumeRenderer[i]->setVisible(true);

		QVariantMap rendererSettings;
		rendererSettings[iARendererImpl::ShowAxesCube] = ShowHelpers;
		rendererSettings[iARendererImpl::ShowOriginIndicator] = ShowHelpers;
		rendererSettings[iARendererImpl::BackgroundBottom] = RendererBackground;
		rendererSettings[iARendererImpl::BackgroundTop] = RendererBackground;
		m_renderers[i]->applySettings(rendererSettings);
		m_rendererManager.addToBundle(m_renderers[i]->renderer());
		setLayout(new QHBoxLayout());
		layout()->addWidget(m_vtkWidgets[i]);
	}
}

dlg_dataView4DCT::~dlg_dataView4DCT()
{
	delete [] m_vtkWidgets;
	delete [] m_renderers;
}

void dlg_dataView4DCT::update()
{
	for(size_t i = 0; i < m_volumeViewers.size(); i++)
	{
		// TODO NEWIO: use datasets!
		// m_renderers[i]->reInitialize(m_volumeStack->volume(i), m_mdiChild->polyData());
		m_renderers[i]->update();
		//m_volumeRenderer[i]->update(); // TODO: VOLUME: check if necessary!
	}
}

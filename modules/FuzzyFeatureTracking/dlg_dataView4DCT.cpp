/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_dataView4DCT.h"

#include "ui_DataView4DCT.h"

#include <iAModalityTransfer.h>
#include <iARenderer.h>
#include <iATransferFunction.h>
#include <iAVolumeRenderer.h>
#include <iAVolumeStack.h>
#include <mdichild.h>
#include <qthelper/iAQTtoUIConnector.h>
#include <iAQVTKWidgetMouseReleaseWorkaround.h>

#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkTransform.h>

const double	FOURDCT_BACGROUND[3]	= {1, 1, 1};
const double	FOURDCT_BACGROUND2[3]	= {1, 1, 1};
const bool		SHOW_HELPERS			= false;

dlg_dataView4DCT::dlg_dataView4DCT(QWidget *parent, iAVolumeStack* volumeStack):
	dlg_dataView4DCTContainer(parent),
	m_axesTransform(vtkSmartPointer<vtkTransform>::New())
{
	m_mdiChild = dynamic_cast<MdiChild*>(parent);
	m_volumeStack = volumeStack;

	m_rendererManager.addToBundle(m_mdiChild->renderer()->renderer());

	// add widgets to window
	int numOfVolumes = m_volumeStack->numberOfVolumes();
	m_vtkWidgets = new iAQVTKWidgetMouseReleaseWorkaround*[numOfVolumes];
	m_renderers = new iARenderer*[numOfVolumes];
	m_volumeRenderer = new iAVolumeRenderer*[numOfVolumes];
	for(int i = 0; i < numOfVolumes; i++)
	{
		m_vtkWidgets[i] = new iAQVTKWidgetMouseReleaseWorkaround(this);
		m_renderers[i] = new iARenderer(this);
		// TODO: VOLUME: check if this is working!
		iASimpleTransferFunction transferFunction(
			m_volumeStack->colorTF(i),
			m_volumeStack->opacityTF(i)
		);
		m_volumeRenderer[i] = new iAVolumeRenderer(&transferFunction, m_volumeStack->volume(i));
		m_renderers[i]->setAxesTransform(m_axesTransform);
		m_vtkWidgets[i]->SetRenderWindow(m_renderers[i]->renderWindow());
		m_renderers[i]->initialize(m_volumeStack->volume(i), m_mdiChild->polyData());
		m_volumeRenderer[i]->addTo(m_renderers[i]->renderer());
		m_renderers[i]->applySettings( m_mdiChild->renderSettings() );
		m_volumeRenderer[i]->applySettings(m_mdiChild->volumeSettings());

		// setup renderers
		m_renderers[i]->showHelpers(SHOW_HELPERS);
		m_renderers[i]->renderer()->SetBackground(FOURDCT_BACGROUND[0], FOURDCT_BACGROUND[1], FOURDCT_BACGROUND[2]);
		m_renderers[i]->renderer()->SetBackground2(FOURDCT_BACGROUND2[0], FOURDCT_BACGROUND2[1], FOURDCT_BACGROUND2[2]);

		m_rendererManager.addToBundle(m_renderers[i]->renderer());

		this->dockWidgetContents->layout()->addWidget(m_vtkWidgets[i]);
	}
}

dlg_dataView4DCT::~dlg_dataView4DCT()
{
	delete m_vtkWidgets;
	delete m_renderers;
}

void dlg_dataView4DCT::update()
{
	for(int i = 0; i < m_volumeStack->numberOfVolumes(); i++)
	{
		m_renderers[i]->reInitialize(m_volumeStack->volume(i), m_mdiChild->polyData());
		m_renderers[i]->update();
		m_volumeRenderer[i]->update(); // TODO: VOLUME: check if necessary!
	}
}

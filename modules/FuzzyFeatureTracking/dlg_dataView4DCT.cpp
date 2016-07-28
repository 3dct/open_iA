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
#include "dlg_dataView4DCT.h"

#include "ui_DataView4DCT.h"
#include "iAQTtoUIConnector.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iATransferFunction.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeStack.h"
#include "QVTKWidgetMouseReleaseWorkaround.h"
#include "mdichild.h"

#include <vtkCamera.h>
#include <vtkPolyData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkRendererCollection.h>
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

	m_rendererManager.addToBundle(m_mdiChild->getRenderer());

	// add widgets to window
	int numOfVolumes = m_volumeStack->getNumberOfVolumes();
	m_vtkWidgets = new QVTKWidgetMouseReleaseWorkaround*[numOfVolumes];
	m_renderers = new iARenderer*[numOfVolumes];
	m_volumeRenderer = new iAVolumeRenderer*[numOfVolumes];
	for(int i = 0; i < numOfVolumes; i++)
	{
		m_vtkWidgets[i] = new QVTKWidgetMouseReleaseWorkaround(this);
		m_renderers[i] = new iARenderer(this);
		// TODO: VOLUME: check if this is working!
		iASimpleTransferFunction transferFunction(
			m_volumeStack->getColorTransferFunction(i),
			m_volumeStack->getPiecewiseFunction(i)
		);
		m_volumeRenderer[i] = new iAVolumeRenderer(&transferFunction, m_volumeStack->getVolume(i));
		m_renderers[i]->setAxesTransform(m_axesTransform);
		m_vtkWidgets[i]->SetRenderWindow(m_renderers[i]->GetRenderWindow());
		m_renderers[i]->initialize(m_volumeStack->getVolume(i), m_mdiChild->getPolyData());
		m_volumeRenderer[i]->AddTo(m_renderers[i]->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
		m_mdiChild->ApplyRenderSettings(m_renderers[i]);
		m_volumeRenderer[i]->ApplySettings(m_mdiChild->GetVolumeSettings());
		
		// setup renderers
		m_renderers[i]->showHelpers(SHOW_HELPERS);
		m_renderers[i]->GetRenderer()->SetBackground(FOURDCT_BACGROUND[0], FOURDCT_BACGROUND[1], FOURDCT_BACGROUND[2]);
		m_renderers[i]->GetRenderer()->SetBackground2(FOURDCT_BACGROUND2[0], FOURDCT_BACGROUND2[1], FOURDCT_BACGROUND2[2]);

		m_rendererManager.addToBundle(m_renderers[i]);
		
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
	for(int i = 0; i < m_volumeStack->getNumberOfVolumes(); i++)
	{
		m_renderers[i]->reInitialize(m_volumeStack->getVolume(i), m_mdiChild->getPolyData());
		m_renderers[i]->update();
		m_volumeRenderer[i]->Update(); // TODO: VOLUME: check if necessary!
	}
}

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
#include "iAVRAttachment.h"

#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAVolumeRenderer.h>

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRCamera.h>

// must be after vtk includes, otherwise -> #error:  gl.h included before glew.h
#include <mdichild.h>

iAVRAttachment::iAVRAttachment( MainWindow * mainWnd, iAChildData childData )
	: iAModuleAttachmentToChild( mainWnd, childData ), 
	m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New()),
	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()),
	m_interactor(vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New()),
	m_camera(vtkSmartPointer<vtkOpenVRCamera>::New())
{
	MdiChild * mdiChild = m_childData.child;
	
	m_renderWindow->AddRenderer(m_renderer);
	m_interactor->SetRenderWindow(m_renderWindow);
	m_renderer->SetActiveCamera(m_camera);

	m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(mdiChild->GetModality(0)->GetTransfer().get(), mdiChild->GetModality(0)->GetImage()));

	m_volumeRenderer->AddTo(m_renderer);


	//iADockWidgetWrapper* wrapper = new iADockWidgetWrapper
	m_renderWindow->Render();
	//m_interactor->Start();
}


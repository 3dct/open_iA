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
#include "iAVREnvironment.h"

#include "iAVRInteractor.h"

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>

iAVREnvironment::iAVREnvironment():
	m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New()),
	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()),
	m_interactor(vtkSmartPointer<iAVRInteractor>::New()),
	m_camera(vtkSmartPointer<vtkOpenVRCamera>::New())
{
	m_renderWindow->AddRenderer(m_renderer);
	// MultiSamples needs to be set to 0 to make Volume Rendering work:
	// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
	m_renderWindow->SetMultiSamples(0);
	m_interactor->SetRenderWindow(m_renderWindow);
	m_renderer->SetActiveCamera(m_camera);
	//auto colors = vtkSmartPointer<vtkNamedColors>::New();
	//colors->GetColor3d("ForestGreen").GetData()
	m_renderer->SetBackground(0, 0, 0);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

void iAVREnvironment::start()
{
	m_renderer->ResetCamera();
	//m_camera->SetPosition();
	m_renderWindow->Render();
	m_interactor->Start();
}

void iAVREnvironment::stop()
{
	m_interactor->stop();
}
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
#include "iAVREnvironment.h"

#include "iAVRInteractor.h"

#include "iAConsole.h"

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>


iAVREnvironment::iAVREnvironment():	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()), m_interactor(vtkSmartPointer<iAVRInteractor>::New()), 
m_renderWindow(vtkSmartPointer<vtkOpenVRRenderWindow>::New())
{	
	m_renderer->SetBackground(50, 50, 50);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

iAVRInteractor* iAVREnvironment::interactor()
{
	return m_interactor;
}

void iAVREnvironment::update()
{
	m_renderWindow->Render();
}

void iAVREnvironment::start()
{
	static int runningInstances = 0;
	// "poor man's" check for trying to run two VR sessions in parallel:
	if (runningInstances >= 1)
	{
		DEBUG_LOG("Cannot start more than one VR session in parallel!");
		emit finished();
		return;
	}
	++runningInstances;
	m_renderWindow->AddRenderer(m_renderer);
	// MultiSamples needs to be set to 0 to make Volume Rendering work:
	// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
	m_renderWindow->SetMultiSamples(0);
	m_interactor->SetRenderWindow(m_renderWindow);
	auto camera = vtkSmartPointer<vtkOpenVRCamera>::New();

	m_renderer->SetActiveCamera(camera);
	m_renderer->ResetCamera();
	m_renderer->ResetCameraClippingRange();
	m_renderWindow->Render();
	m_interactor->Start();
	--runningInstances;
	emit finished();
}

void iAVREnvironment::stop()
{
	if (m_interactor)
		m_interactor->stop();
}
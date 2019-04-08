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

#include "iAConsole.h"
#include "iAVRInteractor.h"

#include "iAConsole.h"

#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRCamera.h>

#include <QThread>

class iAVRMainThread : public QThread
{
public:
	iAVRMainThread(vtkSmartPointer<vtkOpenVRRenderer> ren):
		m_renderer(ren)
	{}
	void run() override
	{
		auto renderWindow = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
		renderWindow->AddRenderer(m_renderer);
		// MultiSamples needs to be set to 0 to make Volume Rendering work:
		// http://vtk.1045678.n5.nabble.com/Problems-in-rendering-volume-with-vtkOpenVR-td5739143.html
		renderWindow->SetMultiSamples(0);
		m_interactor = vtkSmartPointer<iAVRInteractor>::New();
		m_interactor->SetRenderWindow(renderWindow);
		auto camera = vtkSmartPointer<vtkOpenVRCamera>::New();

		m_renderer->SetActiveCamera(camera);
		m_renderer->ResetCamera();
		renderWindow->Render();
		m_interactor->Start();
	}
	void stop()
	{
		m_interactor->stop();
	}
private:
	vtkSmartPointer<vtkOpenVRRenderer> m_renderer;
	vtkSmartPointer<iAVRInteractor> m_interactor;
};

iAVREnvironment::iAVREnvironment():
	m_renderer(vtkSmartPointer<vtkOpenVRRenderer>::New()),
	m_vrMainThread(nullptr)
{
	m_renderer->SetBackground(50, 50, 50);
}

vtkRenderer* iAVREnvironment::renderer()
{
	return m_renderer;
}

void iAVREnvironment::start()
{
	if (m_vrMainThread)
	{
		DEBUG_LOG("VR Environment already started!");
		return;
	}
	m_vrMainThread = new iAVRMainThread(m_renderer);
	connect(m_vrMainThread, &QThread::finished, this, &iAVREnvironment::vrDone);
	m_vrMainThread->start();
	--runningInstances;
	emit finished();
}

void iAVREnvironment::stop()
{
	if (!m_vrMainThread)
	{
		DEBUG_LOG("VR Environment not running!");
		return;
	}
	if (m_vrMainThread)
		m_vrMainThread->stop();
}

bool iAVREnvironment::isRunning() const
{
	return m_vrMainThread;
}

void iAVREnvironment::vrDone()
{
	delete m_vrMainThread;
	m_vrMainThread = nullptr;
}

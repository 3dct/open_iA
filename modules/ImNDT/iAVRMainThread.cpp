// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRMainThread.h"

#include <iALog.h>

iAVRMainThread::iAVRMainThread(iAvtkVRRenderWindow* renderWindow, iAvtkVRRenderWindowInteractor* interactor,iAvtkVR::Backend backend) :
	m_renderWindow(renderWindow), m_interactor(interactor), m_backend(backend), m_done(false)
{}

void iAVRMainThread::run()
{
	emit started();
	iAvtkVR::setActionManifest(m_interactor, m_backend);
	m_renderWindow->Initialize();
	//if (!vr::VRInput())
	//{
	//	m_msg = "Headset not available or turned off. Please attach, turn on and try again!";
	//	LOG(lvlWarn, "Headset not available or turned off. Please attach, turn on and try again!");
	//	return;
	//}
	LOG(lvlInfo, QString("VR rendering started (backend: %1)!").arg(iAvtkVR::backendName(m_backend)));
	m_renderWindow->Render();
	// use of vtk's event loop is potentially problematic - calling SetDone on it from another
	// thread causes potential assertion failures in the immediately invoked event:
	// m_interactor->Start();
	// Workaround - custom event loop (we need that for the thread-safe execution of tasks such as the removal of renderers now anyway):
	while (!m_done)
	{
		if (m_tasks.size() > 0)  // no lock for better performance: reading an incorrect value here doesn't hurt,
		{                        // we will just do the task in the next loop cycle
			std::lock_guard<std::mutex> g(m_tasksMutex);
			for (auto t : m_tasks)
			{
				t();
			}
			m_tasks.clear();
		}
		m_interactor->ProcessEvents();
		if (m_interactor->GetDone())
		{
			m_done = true;
		}
	}
	LOG(lvlInfo, "VR rendering has shut down!");
	m_renderWindow->Finalize();
}

void iAVRMainThread::stop()
{
	// can cause assertion failure: `cannot dereference value-initialized vector<bool> iterator` -> probably not thread safe?
	//m_interactor->SetDone(true);
	m_done = true;
}

QString iAVRMainThread::message() const
{
	return m_msg;
}

void iAVRMainThread::queueTask(std::function<void()> task)
{
	std::lock_guard<std::mutex> g(m_tasksMutex);
	m_tasks.push_back(task);
}

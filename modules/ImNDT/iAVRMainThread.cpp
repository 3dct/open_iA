// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRMainThread.h"

#include <iADataSetRenderer.h>
#include <iALog.h>

iAVRMainThread::iAVRMainThread(iAvtkVRRenderWindow* renderWindow, iAvtkVRRenderWindowInteractor* interactor) :
	m_renderWindow(renderWindow), m_interactor(interactor), m_done(false)
{}

void iAVRMainThread::run()
{
	m_renderWindow->Initialize();
	//if (!vr::VRInput())
	//{
	//	m_msg = "Headset not available or turned off. Please attach, turn on and try again!";
	//	LOG(lvlWarn, "Headset not available or turned off. Please attach, turn on and try again!");
	//	return;
	//}
	LOG(lvlInfo, "VR rendering started!");
	m_renderWindow->Render();
	iAVRObjectFactory::setActionManifest(m_interactor);
	// use of vtk's event loop is potentially problematic - calling SetDone on it from another
	// thread causes potential assertion failures in the immediately invoked event:
	// m_interactor->Start();
	// Workaround - custom event loop (we need that for the thread-safe removal of renderers now anyway):
	while (!m_done)
	{
		if (m_renderersToRemove.size() > 0)  // no lock for better performance: reading an incorrect value here doesn't hurt,
		{                                    // we will just remove the renderer in the next loop cycle
			std::lock_guard<std::mutex> g(m_removeMutex);
			for (auto r : m_renderersToRemove)
			{
				r->setVisible(false);
			}
			m_renderersToRemove.clear();
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

void iAVRMainThread::removeRenderer(std::shared_ptr<iADataSetRenderer> renderer)
{
	std::lock_guard<std::mutex> g(m_removeMutex);
	m_renderersToRemove.push_back(renderer);
}

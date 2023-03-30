// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRMainThread.h"

#include <iALog.h>

#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>

#include <QCoreApplication>
#include <QDir>

iAVRMainThread::iAVRMainThread(vtkSmartPointer<vtkOpenVRRenderWindow> renderWindow, vtkSmartPointer<vtkOpenVRRenderWindowInteractor> interactor) :
	m_renderWindow(renderWindow), m_interactor(interactor)
{
	connect(this, &iAVRMainThread::stopSignal, this, &iAVRMainThread::stopSlot, Qt::QueuedConnection);
}
void iAVRMainThread::run()
{
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	// for correct input manifest json path
	auto prevWorkingDir = QDir::currentPath();
	QDir::setCurrent(QCoreApplication::applicationDirPath() + "/VR-input-manifests");
#endif
	m_renderWindow->Initialize();
	if (!vr::VRInput())
	{
		m_msg = "Headset not available or turned off. Please attach, turn on and try again!";
		LOG(lvlWarn, "Headset not available or turned off. Please attach, turn on and try again!");
		return;
	}
	LOG(lvlInfo, "VR rendering started!");
	m_renderWindow->Render();
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	QDir::setCurrent(prevWorkingDir);
#endif
	m_interactor->Start();
	LOG(lvlInfo, "VR rendering has shut down!");
	m_renderWindow->Finalize();
}

void iAVRMainThread::stop()
{
	emit stopSignal();
}

QString iAVRMainThread::message() const
{
	return m_msg;
}

void iAVRMainThread::stopSlot()
{
	m_interactor->SetDone(true);
}

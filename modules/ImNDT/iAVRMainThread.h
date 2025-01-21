// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAvtkVR.h"

#include <QString>
#include <QThread>

#include <mutex>

class iADataSetRenderer;

class iAVRMainThread : public QThread
{
	Q_OBJECT
public:
	iAVRMainThread(iAvtkVRRenderWindow* renderWindow, iAvtkVRRenderWindowInteractor* interactor, iAvtkVR::Backend backend);
	void run() override;
	void stop();
	QString message() const;
	//! queue a task to be executed within the main VR thread
	void queueTask(std::function<void()> task);
signals:
	void started();
private:
	iAvtkVRRenderWindow* m_renderWindow;
	iAvtkVRRenderWindowInteractor* m_interactor;
	std::vector<std::function<void()>> m_tasks;
	std::mutex m_tasksMutex;
	QString m_msg;
	iAvtkVR::Backend m_backend;
	volatile bool m_done;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAvtkVR.h"

#include <vtkSmartPointer.h>

#include <QString>
#include <QThread>

#include <mutex>

class iADataSetRenderer;

class iAVRMainThread : public QThread
{
	Q_OBJECT
public:
	iAVRMainThread(iAvtkVRRenderWindow* renderWindow, iAvtkVRRenderWindowInteractor* interactor);
	void run() override;
	void stop();
	QString message() const;
	void removeRenderer(std::shared_ptr<iADataSetRenderer> renderer);
private:
	iAvtkVRRenderWindow* m_renderWindow;
	iAvtkVRRenderWindowInteractor* m_interactor;
	std::vector<std::shared_ptr<iADataSetRenderer>> m_renderersToRemove;
	std::mutex m_removeMutex;
	QString m_msg;
	volatile bool m_done;
};

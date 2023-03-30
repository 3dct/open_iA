// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QString>
#include <QThread>

class vtkOpenVRRenderWindow;
class vtkOpenVRRenderWindowInteractor;

class iAVRMainThread : public QThread
{
	Q_OBJECT
public:
	iAVRMainThread(vtkSmartPointer<vtkOpenVRRenderWindow> renderWindow, vtkSmartPointer<vtkOpenVRRenderWindowInteractor> interactor);
	void run() override;
	void stop();
	QString message() const;
	void stopSlot();
signals:
	void stopSignal();
private:
	vtkSmartPointer<vtkOpenVRRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkOpenVRRenderWindowInteractor> m_interactor;
	QString m_msg;
};
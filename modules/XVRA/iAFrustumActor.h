// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QElapsedTimer>

#include <memory>
#include <mutex>

class iACameraVis;

class vtkCamera;
class vtkRenderer;

//! Tracks the frustum of a given camera and displays in the given renderer.
class iAFrustumActor: public QObject, public vtkCommand
{
	Q_OBJECT
public:
	iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size);
	~iAFrustumActor();
	void show();
	void hide();
	void Execute(vtkObject*, unsigned long, void*) override;
	//! Updates the source information from the changed camera information.
	//! Call from the outside, in the "GUI" thread (the thread in which the render method is called),
	//! before a call to render, if updateRequired was triggered
	void updateSource();
signals:
	//! triggered whenever the source camera has changed and therefore an update is required
	void updateRequired();

private:
	std::mutex m_mutex;  // for exclusive access to the data
	vtkCamera* m_cam;
	QElapsedTimer m_lastUpdate;
	std::unique_ptr<iACameraVis> m_cameraVis;
};

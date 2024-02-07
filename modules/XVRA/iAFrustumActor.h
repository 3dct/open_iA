// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkActor;
class vtkCamera;
class vtkLineSource;
class vtkRenderer;
class vtkSphereSource;

#include "iAVec3.h"

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QElapsedTimer>

#include <mutex>

//! Tracks the frustum of a given camera and displays in the given renderer.
class iAFrustumActor: public QObject, public vtkCommand
{
	Q_OBJECT
public:
	iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size);
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
	vtkRenderer* m_ren;
	vtkCamera* m_cam;
	double m_size;
	QElapsedTimer m_lastUpdate;
	vtkSmartPointer<vtkActor> m_camPosActor;
	vtkSmartPointer<vtkActor> m_camDirActor;
	vtkSmartPointer<vtkSphereSource> m_camPosSource;
	vtkSmartPointer<vtkLineSource> m_camDirSource;
	iAVec3d m_pos, m_dir;   // for storing the camera's current parameters

	bool m_visible;

	void setupFrustumActor();
};

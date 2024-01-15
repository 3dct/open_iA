// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkActor;
class vtkCamera;
class vtkLineSource;
class vtkRenderer;
class vtkSphereSource;

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

#include <QObject>
#include <QElapsedTimer>

//! Tracks the frustum of a given camera and displays in the given renderer.
class iAFrustumActor: public QObject, public vtkCommand
{
	Q_OBJECT
public:
	iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size);
	void show();
	void hide();
	void Execute(vtkObject*, unsigned long, void*) override;
signals:
	void updateRequired();

private:
	vtkRenderer* m_ren;
	vtkCamera* m_cam;
	double m_size;
	QElapsedTimer m_lastUpdate;
	vtkSmartPointer<vtkActor> m_camPosActor;
	vtkSmartPointer<vtkActor> m_camDirActor;
	vtkSmartPointer<vtkSphereSource> m_camPosSource;
	vtkSmartPointer<vtkLineSource> m_camDirSource;

	bool m_visible;

	void setupFrustumActor();
};

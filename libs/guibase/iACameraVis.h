// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAVec3.h"

#include <vtkSmartPointer.h>

#include <QObject>

class vtkActor;
class vtkCamera;
class vtkLineSource;
class vtkRenderer;
class vtkSphereSource;

class iAguibase_API iACameraVis: public QObject
{
	Q_OBJECT
public:
	iACameraVis(vtkRenderer* ren, double size);
	void show();
	void hide();
	bool update(iAVec3d const & pos, iAVec3d const & dir);
	void updateSource();
	iAVec3d pos() const;
	iAVec3d dir() const;
signals:
	void updateRequired();

private:
	vtkRenderer* m_ren;
	vtkSmartPointer<vtkActor> m_camPosActor;
	vtkSmartPointer<vtkActor> m_camDirActor;
	vtkSmartPointer<vtkSphereSource> m_camPosSource;
	vtkSmartPointer<vtkLineSource> m_camDirSource;
	iAVec3d m_pos, m_dir;  // for storing the camera's current parameters
	double m_size;
	bool m_visible;
};
// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAVec3.h"

#include <vtkSmartPointer.h>

#include <QObject>

class vtkAssembly;
class vtkCamera;
class vtkConeSource;
class vtkCylinderSource;
class vtkRenderer;
class vtkSphereSource;

class iAguibase_API iACameraVis: public QObject
{
	Q_OBJECT
public:
	iACameraVis(vtkRenderer* ren, double size);
	~iACameraVis();
	void show();
	void hide();
	bool update(iAVec3d const & pos, iAVec3d const & dir, iAVec3d const & up = iAVec3d(0, 0, 1));
	void updateSource();
	iAVec3d pos() const;
	iAVec3d dir() const;
	iAVec3d up() const;
signals:
	void updateRequired();

private:
	vtkRenderer* m_ren;
	vtkSmartPointer<vtkAssembly> m_assembly;
	vtkSmartPointer<vtkSphereSource> m_camPosSource;
	vtkSmartPointer<vtkConeSource> m_camDirSource;
	vtkSmartPointer<vtkCylinderSource> m_camUpSource;
	iAVec3d m_pos, m_dir, m_up;  // for storing the camera's current parameters
	double m_size;
	bool m_visible;
};

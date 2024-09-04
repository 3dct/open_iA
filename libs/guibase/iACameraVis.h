// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iAVec3.h"

#include <vtkSmartPointer.h>

#include <QColor>
#include <QObject>

class vtkAssembly;
class vtkCamera;
class vtkCylinderSource;
class vtkRenderer;
class vtkSphereSource;

//! Visualizes a representation of a camera; typically used for displaying
//! the viewing position and direction of a collaborator using a different
//! interface for viewing the same dataset
class iAguibase_API iACameraVis: public QObject
{
	Q_OBJECT
public:
	//! Create a visualization for the given renderer, with the given size and colors
	iACameraVis(vtkRenderer* ren, double size, QColor bodyColor, QColor vecColor);
	~iACameraVis();
	void show();
	void hide();
	//! Update the parameters of the visualization (can be called from any thread)
	bool update(iAVec3d const & pos, iAVec3d const & dir, iAVec3d const & up);
	//! Updates the source and triggers re-rendering (needs to be called within the UI thread)
	void updateSource();
	//! @{ access to the parameters of the visualization
	iAVec3d pos() const;
	iAVec3d dir() const;
	iAVec3d up() const;
	//! @}
signals:
	//! Triggered on a call to update; intended to be used to call updateSource in the UI thread
	void updateRequired();

private:
	//! @{ vtk visualization classes
	vtkRenderer* m_ren;
	vtkSmartPointer<vtkAssembly> m_assembly;
	vtkSmartPointer<vtkSphereSource> m_camPosSource;
	vtkSmartPointer<vtkCylinderSource> m_camDirSource;
	vtkSmartPointer<vtkCylinderSource> m_camUpSource;
	//! @}
	iAVec3d m_pos, m_dir, m_up;  //!< for storing the camera's current parameters
	double m_size;
	bool m_visible;
};

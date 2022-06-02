/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
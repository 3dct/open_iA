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
#include "iAVR3DObjectVis.h"

#include "vtkCubeSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"

iAVR3DObjectVis::iAVR3DObjectVis(vtkRenderer* ren):m_renderer(ren),m_actor(vtkSmartPointer<vtkActor>::New())
{
	m_visible = false;
}

void iAVR3DObjectVis::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_actor);
	m_visible = true;
}

void iAVR3DObjectVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_actor);
	m_visible = false;
}

void iAVR3DObjectVis::createCube(QColor col, double size[3], double center[3])
{
	// Create a cube.
    vtkNew<vtkCubeSource> cube;
	cube->SetXLength(size[0]);
	cube->SetYLength(size[0]);
	cube->SetZLength(size[0]);
	cube->SetCenter(center);
    cube->Update();

    // mapper
    vtkNew<vtkPolyDataMapper> cubeMapper;
    cubeMapper->SetInputConnection(cube->GetOutputPort());

	m_dataSet = cubeMapper->GetInput();

    // Actor.
	m_actor->SetMapper(cubeMapper);
    m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
}

void iAVR3DObjectVis::createSphere(QColor col)
{
	// Create a cube.
	vtkNew<vtkSphereSource> sphere;
	sphere->SetPhiResolution(60);
	sphere->SetThetaResolution(60);
	sphere->SetRadius(300.0);
	sphere->SetCenter(392, 371, 1340);
	sphere->Update();

	// mapper
	vtkNew<vtkPolyDataMapper> sphereMapper;
	sphereMapper->SetInputConnection(sphere->GetOutputPort());

	m_dataSet = sphereMapper->GetInput();

	// Actor.
	m_actor->SetMapper(sphereMapper);
	m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
	m_actor->GetProperty()->SetPointSize(20.0);
	m_actor->GetProperty()->SetRepresentationToPoints();
}

void iAVR3DObjectVis::setScale(double x, double y, double z)
{
	m_actor->SetScale(x, y, z);
}

vtkDataSet * iAVR3DObjectVis::getDataSet()
{
	return m_dataSet;
}

vtkActor * iAVR3DObjectVis::getActor()
{
	return m_actor;
}




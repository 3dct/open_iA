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

void iAVR3DObjectVis::createCube(QColor col)
{
	// Create a cube.
    vtkNew<vtkCubeSource> cube;
	cube->SetXLength(200.0);
	cube->SetYLength(200.0);
	cube->SetZLength(200.0);
	cube->SetCenter(0.0,0.0,0.0);
    cube->Update();

    // mapper
    vtkNew<vtkPolyDataMapper> cubeMapper;
    cubeMapper->SetInputConnection(cube->GetOutputPort());

    // Actor.
    m_actor->SetMapper(cubeMapper);
    m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
}

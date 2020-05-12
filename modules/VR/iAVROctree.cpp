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
#include "iAVROctree.h"

#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include <iAConsole.h>

iAVROctree::iAVROctree(vtkRenderer* ren, vtkDataSet* dataSet):m_renderer(ren),m_dataSet(dataSet),m_actor(vtkSmartPointer<vtkActor>::New()),
m_octree(vtkSmartPointer<vtkOctreePointLocator>::New())
{
	m_visible = false;
}

void iAVROctree::generateOctree(int level, QColor col)
{
	m_octree->SetMaximumPointsPerRegion(25); // The maximum number of points in a region/octant before it is subdivided
	m_octree->SetDataSet(m_dataSet);
	m_octree->BuildLocator();

	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	m_octree->GenerateRepresentation(level, polydata);

	vtkSmartPointer<vtkPolyDataMapper> octreeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	octreeMapper->SetInputData(polydata);
	
	m_actor->SetMapper(octreeMapper);
	//m_actor->GetProperty()->SetInterpolationToFlat();
	m_actor->GetProperty()->SetRepresentationToWireframe();
	//m_actor->GetProperty()->SetRepresentationToPoints();
	m_actor->GetProperty()->SetColor(col.redF(), col.greenF(), col.blueF());
	m_actor->GetProperty()->SetLineWidth(6); //ToDo Use TubeFilter?
	//m_actor->PickableOff();
}

void iAVROctree::FindClosestNPoints(int N, const double x[3], vtkIdList* result)
{
	m_octree->FindClosestNPoints(N, x, result);
}

vtkIdType iAVROctree::FindClosestPoint(const double x[3])
{
	return m_octree->FindClosestPoint(x);
}

vtkOctreePointLocator * iAVROctree::getOctree()
{
	return m_octree;
}

int iAVROctree::GetLevel()
{
	return m_octree->GetMaxLevel();
}

void iAVROctree::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_actor);
	m_visible = true;
}

void iAVROctree::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_actor);
	m_visible = false;
}

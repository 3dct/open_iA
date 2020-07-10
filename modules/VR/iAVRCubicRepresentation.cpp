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
#include "iAVRCubicRepresentation.h"

#include <iAConsole.h>

#include "vtkCellPicker.h"
#include "vtkIdTypeArray.h"
#include "vtkVertexGlyphFilter.h"
#include <vtkPolyDataMapper.h>
#include <vtkAlgorithmOutput.h>
#include "vtkPointData.h"
#include "vtkProperty.h"


iAVRCubicRepresentation::iAVRCubicRepresentation(vtkRenderer* ren) :m_renderer(ren), m_actor(vtkSmartPointer<vtkActor>::New()), m_activeActor(vtkSmartPointer<vtkActor>::New())
{
	m_visible = false;
}

void iAVRCubicRepresentation::setOctree(iAVROctree* octree)
{
	m_octree = octree;
}

void iAVRCubicRepresentation::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_actor);
	m_visible = true;
}

void iAVRCubicRepresentation::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_actor);
	m_visible = false;
}

vtkSmartPointer<vtkActor> iAVRCubicRepresentation::getActor()
{
	return m_actor;
}

vtkSmartPointer<vtkPolyData> iAVRCubicRepresentation::getDataSet()
{
	return m_cubePolyData;
}

//! This Method returns the closest cell of the MiM which gets intersected by a ray  
vtkIdType iAVRCubicRepresentation::getClosestCellID(double pos[3], double eventOrientation[3])
{
	vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();

	if (cellPicker->Pick3DRay(pos, eventOrientation, m_renderer) >= 0)
	{
		vtkIdType regionId = dynamic_cast<vtkIdTypeArray*>(glyph3D->GetOutput()->GetPointData()->GetArray("InputPointIds"))
			->GetValue(cellPicker->GetPointId());

		return regionId;
	}
	return -1;
}

//! This Method iterates through all leaf regions of the octree and stores its center point in an vtkPolyData
void iAVRCubicRepresentation::calculateStartPoints()
{
	vtkSmartPointer<vtkPoints> cubeStartPoints = vtkSmartPointer<vtkPoints>::New();
	m_cubePolyData = vtkSmartPointer<vtkPolyData>::New();

	int leafNodes = m_octree->getOctree()->GetNumberOfLeafNodes();

	for (int i = 0; i < leafNodes; i++)
	{

		//vtkIdTypeArray* points = m_octree->getOctree()->GetPointsInRegion(i);

		////If points are null skip this 'empty' cube
		//if (points->GetNumberOfValues() == 0) {
		//	DEBUG_LOG(QString("EMPTY Cube in Region %1 found").arg(i));
		//	continue;
		//}

		double centerPoint[3];
		m_octree->calculateOctreeRegionCenterPos(i, centerPoint);

		cubeStartPoints->InsertNextPoint(centerPoint[0], centerPoint[1], centerPoint[2]);
	}

	m_cubePolyData->SetPoints(cubeStartPoints);
}

//! Test method inserts colored point at given Position
void iAVRCubicRepresentation::drawPoint(std::vector<double*>* pos, QColor color)
{
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

	for (int i = 0; i < pos->size(); i++)
	{
		points->InsertNextPoint(pos->at(i));
	}

	vtkSmartPointer<vtkPolyData> pointsPolydata = vtkSmartPointer<vtkPolyData>::New();
	pointsPolydata->SetPoints(points);

	vtkSmartPointer<vtkVertexGlyphFilter> vertexGlyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexGlyphFilter->AddInputData(pointsPolydata);
	vertexGlyphFilter->Update();

	vtkSmartPointer<vtkPolyDataMapper> pointsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	pointsMapper->SetInputConnection(vertexGlyphFilter->GetOutputPort());
	vtkSmartPointer<vtkActor> pointsActor = vtkSmartPointer<vtkActor>::New();
	pointsActor->SetMapper(pointsMapper);
	pointsActor->GetProperty()->SetPointSize(8);
	pointsActor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
	m_renderer->AddActor(pointsActor);

}
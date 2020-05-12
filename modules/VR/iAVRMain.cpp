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
#include "iAVRMain.h"

#include <iAConsole.h>
#include "iAVRInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkIdList.h"
#include "vtkProperty.h"
#include "vtkPolyData.h"
#include "iA3DCylinderObjectVis.h"
#include "iAVR3DObjectVis.h"
#include "iAVROctree.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"


#include <vtkVertexGlyphFilter.h>
#include <vtkPolyDataMapper.h>


iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{

	m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(100, 100, 100), std::map<size_t, std::vector<iAVec3f> >());

	//TEST ADD Cube
	//m_objectVis = new iAVR3DObjectVis(m_vrEnv->renderer());
	//m_objectVis->createCube(QColor(0, 190, 50, 255));
	//m_objectVis->createSphere(QColor(0, 190, 50, 255));

	//QSharedPointer<iAVR3DObjectVis> m_objectVis2;
	//m_objectVis2.reset(new iAVR3DObjectVis(m_vrEnv->renderer()));
	//m_objectVis2->createCube(QColor(0, 40, 195, 255));

	//TEST Add octree
	octreeLevel = 1;
	m_octree = new iAVROctree(m_vrEnv->renderer(), m_cylinderVis->getPolyData());
	m_octree->generateOctree(octreeLevel, QColor(126, 0, 223, 255));
	//TEST add InteractorStyle
	m_style->setVRMain(this);
	m_vrEnv->interactor()->SetInteractorStyle(m_style);

	m_cylinderVis->show();
	//m_objectVis->show();
	m_octree->show();


}

void iAVRMain::startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], vtkProp3D* m_pickedProp)
{
	vtkEventDataDeviceInput input = device->GetInput();  // Input Method
	vtkEventDataAction action = device->GetAction();     // Action of Input Method

	if (action == vtkEventDataAction::Press)
	{

		if (input == vtkEventDataDeviceInput::TrackPad)
		{
			if (octreeLevel >= m_octree->GetLevel())
			{
				octreeLevel = 0;
			}
			octreeLevel++;

			DEBUG_LOG(QString("Current Octree Level (Max = %1) is |%2|").arg(m_octree->GetLevel()).arg(octreeLevel));

			m_octree->generateOctree(octreeLevel, QColor(126, 0, 223, 255));
			
		}
		if(input == vtkEventDataDeviceInput::Trigger)
		{
				DEBUG_LOG(QString("Controller Pos: [%1 -- %2 -- %3] \n")
					.arg(eventPosition[0])
					.arg(eventPosition[1])
					.arg(eventPosition[2]));

				if (m_pickedProp != nullptr)
				{
					// Find the closest points to TestPoint
					vtkIdType iD = m_octree->FindClosestPoint(eventPosition);
					DEBUG_LOG(QString("Closest Points are: %1").arg(iD));

					// Get Coord
					double closestPoint[3];
					m_octree->getOctree()->GetDataSet()->GetPoint(iD, closestPoint);

					// Define some colors
					double color[3];
					color[0] = {0.0};
					color[1] = {190.0};
					color[2] = {205.0};

					// Setup the colors array
					vtkSmartPointer<vtkUnsignedCharArray> pointData = vtkSmartPointer<vtkUnsignedCharArray>::New();
					pointData->SetNumberOfComponents(3);
					pointData->SetName("Colors");
					pointData->InsertTuple(iD,color);
					//m_objectVis->getDataSet()->GetPointData()->SetScalars(pointData);

					vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
					points->InsertNextPoint(closestPoint);

					vtkSmartPointer<vtkPolyData> pointsPolydata = vtkSmartPointer<vtkPolyData>::New();
					pointsPolydata->SetPoints(points);

					vtkSmartPointer<vtkVertexGlyphFilter> vertexGlyphFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
					vertexGlyphFilter->AddInputData(pointsPolydata);
					vertexGlyphFilter->Update();

					vtkSmartPointer<vtkPolyDataMapper> pointsMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
					pointsMapper->SetInputConnection(vertexGlyphFilter->GetOutputPort());
					vtkSmartPointer<vtkActor> pointsActor = vtkSmartPointer<vtkActor>::New();
					pointsActor->SetMapper(pointsMapper);
					pointsActor->GetProperty()->SetPointSize(5);
					pointsActor->GetProperty()->SetColor(color);
					m_vrEnv->renderer()->AddActor(pointsActor);

					vtkIdType rowiD = getObjectiD(closestPoint);

					DEBUG_LOG(QString("row iD: %1").arg(rowiD));

					m_cylinderVis->renderSingle(rowiD+1, 0, QColor(100, 100, 100, 255), nullptr);

				}
				else
				{
					DEBUG_LOG(QString("Prop: is null"));
				}
		}
	}

	//Update Changes
	m_vrEnv->update();
}

void iAVRMain::endInteraction()
{
	/*Do NOTHING*/;
}

//! Looks in the vtkTable for the given position (could be start or end position)
//! Returns the row iD of the found entrance in the table
//! Converts double pos to float for use in vtk!
vtkIdType iAVRMain::getObjectiD(double pos[3])
{
	float posf[3];
	// Convert to float
	for (int i = 0; i < 3; ++i)
	{
		posf[i] = (float)(pos[i]);
	}

	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{

		float startPos[3], endPos[3];
		for (int i = 0; i < 3; ++i)
		{
			startPos[i] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + i)).ToFloat();
			endPos[i] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + i)).ToFloat();
		}

		if (checkEqualArrays(posf, startPos) || checkEqualArrays(posf, endPos))
		{
			return row;
		}

	}

	return -10000;
}

//! Checks if two pos arrays are the same
bool iAVRMain::checkEqualArrays(float pos1[3], float pos2[3])
{
	for (int i = 0; i < 3; ++i)
	{
		if (pos1[i] != pos2[i])
		{
			return false;
		}
	}
	return true;
}


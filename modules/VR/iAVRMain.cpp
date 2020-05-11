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


iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{

	m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(255, 0, 0), std::map<size_t, std::vector<iAVec3f> >());

	//TEST ADD Cube
	m_objectVis = new iAVR3DObjectVis(m_vrEnv->renderer());
	m_objectVis->createCube(QColor(0, 190, 50, 255));

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
	m_objectVis->show();
	m_octree->show();


}

void iAVRMain::startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], vtkProp3D* m_pickedProp)
{
	vtkEventDataDeviceInput input = device->GetInput();  // Input Method
	vtkEventDataAction action = device->GetAction();     // Action of Input Method

	if (action == vtkEventDataAction::Press)
	{
		if (octreeLevel > 3)
		{
			octreeLevel = 0;
		}
		octreeLevel++;

		m_octree->generateOctree(octreeLevel, QColor(126, 0, 223, 255));
		

		DEBUG_LOG(QString("Controller Pos: %1 -- %2 -- %3")
					  .arg(eventPosition[0])
					  .arg(eventPosition[1])
					  .arg(eventPosition[2]));

		if (m_pickedProp != nullptr)
		{
			// Find the closest points to TestPoint
			vtkIdType iD = m_octree->FindClosestPoint(eventPosition);
			DEBUG_LOG(QString("Closest Points are: ").arg(iD));
		}
		else
		{
			DEBUG_LOG(QString("Prop: is null"));
		}
	}

	//Update Changes
	m_vrEnv->update();
}

void iAVRMain::endInteraction()
{
	/*Do NOTHING*/;
}


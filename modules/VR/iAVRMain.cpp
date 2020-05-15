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
#include <vtkIdTypeArray.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPolyDataMapper.h>
#include "iAVRInteractorStyle.h"

#include <QColor>


iAVRMain::iAVRMain(iAVREnvironment* vrEnv, iAVRInteractorStyle* style, vtkTable* objectTable, iACsvIO io): m_vrEnv(vrEnv),
	m_style(style),	m_objectTable(objectTable),	m_io(io)
{
	m_cylinderVis = new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, m_io.getOutputMapping(), QColor(140,140,140,255), std::map<size_t, std::vector<iAVec3f> >());
	m_iDMappingThread = std::thread(&iAVRMain::mapAllPointiDs, this);

	//TEST add InteractorStyle
	m_style->setVRMain(this);
	m_vrEnv->interactor()->SetInteractorStyle(m_style);

	//Add Input Mapping
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickSingleFiber);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::Volume, iAVROperations::PickFibersinRegion);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::TrackPad, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ChangeOctreeLevel);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::Trigger, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);
	this->setInputScheme(vtkEventDataDevice::RightController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::ResetSelection);

	this->setInputScheme(vtkEventDataDevice::LeftController, vtkEventDataDeviceInput::ApplicationMenu, vtkEventDataAction::Press,
		iAVRInteractionOptions::NoObject, iAVROperations::SpawnModelInMiniature);

	//Create Cube
	m_objectVis = new iAVR3DObjectVis(m_vrEnv->renderer());

	//TEST Add octree
	octreeLevel = 1;
	m_octree = new iAVROctree(m_vrEnv->renderer(), m_cylinderVis->getPolyData());
	m_octree->generateOctree(octreeLevel, QColor(126, 0, 223, 255));

	m_cylinderVis->show();	
	m_octree->show();

}

//! Defines the action executed for specific controller inputs
void iAVRMain::startInteraction(vtkEventDataDevice3D* device, double eventPosition[3], vtkProp3D* pickedProp)
{
	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction());     // Action of Input Method
	int optionID = getOptionForObject(pickedProp);

	// Active Input Saves the current applied operation in case Multiinput is requires
	std::vector<int>* activeInput = m_style->getActiveInput();

	inputScheme* scheme = m_style->getInputScheme();
	int operation = scheme->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		DEBUG_LOG(QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		break;
	case iAVROperations::SpawnModelInMiniature:
		this->spawnModelInMiniature(eventPosition, false);
		activeInput->at(deviceID) = operation;
		break;
	case iAVROperations::PickSingleFiber:
		this->pickSingleFiber(eventPosition);
		activeInput->at(deviceID) = operation;
		break;
	case iAVROperations::PickFibersinRegion:
		this->pickFibersinRegion(eventPosition);
		activeInput->at(deviceID) = operation;
		break;
	case iAVROperations::ChangeOctreeLevel:
		this->changeOctreeLevel();
		activeInput->at(deviceID) = operation;
		break;
	case iAVROperations::ResetSelection:
		this->resetSelection();
		activeInput->at(deviceID) = operation;
		break;
	}

	//Update Changes //ToDO: Required? Or is render Selection enough?
	m_vrEnv->update();
}

void iAVRMain::endInteraction(vtkEventDataDevice3D* device, double eventPosition[3], vtkProp3D* pickedProp)
{
	int deviceID = static_cast<int>(device->GetDevice()); // Device
	int inputID = static_cast<int>(device->GetInput());  // Input Method
	int actioniD = static_cast<int>(device->GetAction());     // Action of Input Method
	int optionID = getOptionForObject(pickedProp);

	// Active Input Saves the current applied operation in case Multiinput is requires
	std::vector<int>* activeInput = m_style->getActiveInput();

	inputScheme* scheme = m_style->getInputScheme();
	int operation = scheme->at(deviceID).at(inputID).at(actioniD).at(optionID);

	switch (iAVROperations(operation))
	{
	case iAVROperations::Unknown:
		DEBUG_LOG(QString("Unknown Operation!"));
		break;
	case iAVROperations::None:
		break;
	case iAVROperations::SpawnModelInMiniature:
		this->spawnModelInMiniature(eventPosition, true);
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::None);
		break;
	case iAVROperations::PickSingleFiber:
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::None);
		break;
	case iAVROperations::PickFibersinRegion:
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::None);
		break;
	case iAVROperations::ChangeOctreeLevel:
		activeInput->at(deviceID) = static_cast<int>(iAVROperations::None);
		break;
	case iAVROperations::ResetSelection:
		activeInput->at(deviceID) = operation;
		break;
	}

	//Update Changes //ToDO: Required? Or is render Selection enough?
	m_vrEnv->update();
}

//! Conputes which polyObject ID (points) belongs to which Object ID in the csv file of the volume
//! Gets only called internally from thread to store the mapping
void iAVRMain::mapAllPointiDs()
{
	//m_cylinderVis->getPolyData()->GetPoints()->GetPoint(5904); // TEST TEST
	//DEBUG_LOG(QString("Size of Array #: %1 ").arg(polyPoints->GetData()->GetSize())); //ToDo Why is the Array bigger then GetNumberOfPoints

	vtkPoints* polyPoints = m_cylinderVis->getPolyData()->GetPoints();

	//For all points in vtkPolyData store their pos
	for (int i = 0; i < polyPoints->GetNumberOfPoints(); i++)
	{
		double temp_pos[3];
		float temp_posf[3];

		polyPoints->GetPoint(i, temp_pos);
		//Conversion
		for (int j = 0; j < 3; ++j)
		{
			temp_posf[j] = (float)(temp_pos[j]);
		}
		
		// Check csv table
		for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
		{

			float startPos[3], endPos[3];
			for (int k = 0; k < 3; ++k)
			{
				startPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::StartX + k)).ToFloat();
				endPos[k] = m_objectTable->GetValue(row, m_io.getOutputMapping()->value(iACsvConfig::EndX + k)).ToFloat();
			}

			// If start or end is equal then the poly point gets the corresponding object ID
			if (checkEqualArrays(temp_posf, startPos) || checkEqualArrays(temp_posf, endPos))
			{
				m_pointIDToCsvIndex.insert(std::make_pair(i, row));
				break;
			}

		}

	}
	DEBUG_LOG(QString("Volume Data loaded"));
	m_iDMappingThreadRunning = false; //Thread ended
}

//! Looks in the vtkTable for the given position (could be start or end position)
//! Returns the row iD of the found entrance in the table
//! Converts double pos to float for use in vtk! Returns -1 if point is not found in csv
vtkIdType iAVRMain::mapSinglePointiD(vtkIdType polyPoint)
{
	double pos[3];
	m_octree->getOctree()->GetDataSet()->GetPoint(polyPoint, pos);

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

	return -1;
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

void iAVRMain::setInputScheme(vtkEventDataDevice device, vtkEventDataDeviceInput input, vtkEventDataAction action, iAVRInteractionOptions options, iAVROperations operation)
{
	inputScheme* scheme = m_style->getInputScheme();

	scheme->at(static_cast<int>(device)).at(static_cast<int>(input)).at(static_cast<int>(action)).at(static_cast<int>(options)) = static_cast<int>(operation);
}

//! Returns which InteractionOption is for the currently picked Object available 
int iAVRMain::getOptionForObject(vtkProp3D* pickedProp)
{
	if (pickedProp == nullptr)
	{
		return static_cast<int>(iAVRInteractionOptions::NoObject);
	}
	else
	{
		return static_cast<int>(iAVRInteractionOptions::Volume); //ToDo change Options
	}
}

//! If mapping thread has finsihed load data from map, otherwise calculate it
//! Returns -1 if point is not found in csv
vtkIdType iAVRMain::getObjectiD(vtkIdType polyPoint)
{
	if (!m_iDMappingThreadRunning)
	{
		if (m_iDMappingThread.joinable())
		{
			m_iDMappingThread.join();
		}

		if (m_pointIDToCsvIndex.find(polyPoint) != m_pointIDToCsvIndex.end())
		{
			return m_pointIDToCsvIndex.at(polyPoint);
		}
		else
		{
			DEBUG_LOG(QString("Point not found in csv"));
			return -1;
		}

	}
	else
	{
		return mapSinglePointiD(polyPoint);
	}

}

void iAVRMain::changeOctreeLevel()
{
	if (octreeLevel >= 3)
	{
		octreeLevel = 0;
	}
	octreeLevel++;

	m_octree->generateOctree(octreeLevel, QColor(126, 0, 223, 255));
}

void iAVRMain::pickSingleFiber(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();
	// Find the closest points to TestPoint
	vtkIdType iD = m_octree->FindClosestPoint(eventPosition);
	/*
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
	*/

	vtkIdType rowiD = getObjectiD(iD);
	selection.push_back(rowiD);

	m_cylinderVis->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
}

void iAVRMain::pickFibersinRegion(double eventPosition[3])
{
	std::vector<size_t> selection = std::vector<size_t>();

	int leafRegion = m_octree->getOctree()->GetRegionContainingPoint(eventPosition[0], eventPosition[1], eventPosition[2]);
	vtkIdTypeArray *points = m_octree->getOctree()->GetPointsInRegion(leafRegion);

	//Check if points is null!!
	if (points == nullptr) {
		DEBUG_LOG(QString("No points in the region!"));
		return;
	}

	for (int i = 0; i < points->GetSize(); i++)
	{
		vtkIdType rowiD = getObjectiD(points->GetValue(i));
		selection.push_back(rowiD);
	}

	std::sort(selection.begin(), selection.end());
	selection.erase(std::unique(selection.begin(), selection.end()), selection.end());

	m_cylinderVis->renderSelection(selection, 0, QColor(140, 140, 140, 255), nullptr);
	
}
void iAVRMain::resetSelection()
{
	m_cylinderVis->renderSelection(std::vector<size_t>(), 0, QColor(140, 140, 140, 255), nullptr);
}

void iAVRMain::spawnModelInMiniature(double eventPosition[3], bool hide)
{
	if(!hide)
	{
		double size[3] = { 100.0, 100.0, 100.0 };
		double controllerCenter[3] = { eventPosition[0] + size[0], eventPosition[1] + size[1], eventPosition[2]};
		m_objectVis->createCube(QColor(0, 190, 50, 255), size, controllerCenter);
		m_objectVis->show();
	}
	else
	{
		m_objectVis->hide();
	}

}

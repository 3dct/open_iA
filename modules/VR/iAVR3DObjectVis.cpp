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
#include "vtkPropAssembly.h"
#include "vtkIdFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"
#include "vtkColorTransferFunction.h"
#include "vtkNamedColors.h"
#include "vtkCellPicker.h"
#include "vtkProp3DCollection.h"

#include <iAConsole.h>
#include <math.h>

iAVR3DObjectVis::iAVR3DObjectVis(vtkRenderer* ren):m_renderer(ren),m_actor(vtkSmartPointer<vtkActor>::New())
{
	m_visible = false;
	defaultColor = QColor(0, 0, 200, 200); // Not fully opaque
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

//! Creates for every region of the octree a cube glyph. The cubes are stored in one actor with the set default color.
void iAVR3DObjectVis::createModelInMiniature()
{
	int leafNodes = m_octree->getOctree()->GetNumberOfLeafNodes();
	if (leafNodes <= 0)
	{
		DEBUG_LOG(QString("The Octree has no leaf node!"));
		return;
	}
	
	double regionSize[3] = { 0.0, 0.0, 0.0 };
	m_octree->calculateOctreeRegionSize(regionSize);
	calculateStartPoints();

	// Create anything you want here, we will use a cube for the demo.
	vtkSmartPointer<vtkCubeSource> cubeSource =	vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->SetXLength(regionSize[0]);
	cubeSource->SetYLength(regionSize[1]);
	cubeSource->SetZLength(regionSize[2]);
	cubeSource->Update();

	//glyph3Dmapper = vtkSmartPointer<vtkGlyph3DMapper>::New();
	//glyph3Dmapper->GeneratePointIdsOn(); Not supported as vtkGlyph3DMapper only as vtkGlyph3D
	//glyph3Dmapper->SetSourceConnection(cubeSource->GetOutputPort());
	//glyph3Dmapper->SetInputData(m_cubePolyData);
	//glyph3Dmapper->SetScalarModeToUseCellData();
	//glyph3Dmapper->UseSelectionIdsOn();
	//glyph3Dmapper->SetSelectionIdArray("OctreeRegionID");
	//glyph3Dmapper->Update();

	glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	glyph3D->GeneratePointIdsOn();
	glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	glyph3D->SetInputData(m_cubePolyData);
	glyph3D->SetColorModeToColorByScalar();
	glyph3D->SetScaleModeToDataScalingOff();
	glyph3D->Update();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

	m_actor->SetMapper(glyphMapper);
	m_actor->GetProperty()->SetColor(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF());
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

void iAVR3DObjectVis::setPos(double x, double y, double z)
{
	m_actor->SetPosition(x, y, z);
}

void iAVR3DObjectVis::setOrientation(double x, double y, double z)
{
	m_actor->SetOrientation(x, y, z);
}

//! Sets the color (rgba) of a cube in the Miniature Model
//! The region ID of the octree is used
void iAVR3DObjectVis::setCubeColor(QColor col, int regionID)
{
	unsigned char default_rgb[4] = { 0, 0, 250, 255 };
	unsigned char rgb[4] = {col.red(), col.green(), col.blue(), col.alpha()};

	vtkSmartPointer<vtkUnsignedCharArray> colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetName("colors");
	colors->SetNumberOfComponents(4);

	for(int i = 0; i< m_octree->getOctree()->GetNumberOfLeafNodes(); i++)
	{
		if(i == regionID)
		{
			colors->InsertNextTuple4(rgb[0], rgb[1], rgb[2], rgb[3]);
		}
		else
		{
			colors->InsertNextTuple4(default_rgb[0], default_rgb[1], default_rgb[2], default_rgb[3]);
		}
	}

	m_cubePolyData->GetPointData()->SetScalars(colors); //If Mapper Cell Data is needed!
	m_cubePolyData->Modified();
	glyph3D->Update();
}

void iAVR3DObjectVis::setLinearCubeOffset(double offset)
{
	if(m_cubePolyData == nullptr)
	{
		DEBUG_LOG(QString("No Points to apply offset"));
		return;
	}

	double* pos = m_actor->GetCenter();
	/*
	for (int i = 0; i < m_cubePolyData->GetNumberOfPoints(); i++)
	{
		double* currentPoint = m_cubePolyData->GetPoints()->GetPoint(i);
		double direction[3];
		direction[0] = currentPoint[0] - pos[0];
		direction[1] = currentPoint[1] - pos[1];
		direction[2] = currentPoint[2] - pos[2];
		double length = sqrt(pow(direction[0], 2) + pow(direction[1], 2) + pow(direction[2], 2));
		double normalized[3] = { (direction[0] / length), (direction[1] / length) , (direction[2] / length)};
		double move[3] = { currentPoint[0] * (normalized[0] * offset), currentPoint[0] * (normalized[1] * offset) , currentPoint[0] * (normalized[2] * offset)};

		m_cubePolyData->GetPoints()->SetPoint(i, move);
		m_cubePolyData->Modified();
	}
	*/
	DEBUG_LOG(QString("Linear Offset Set"));
}

vtkIdType iAVR3DObjectVis::getClosestCellID(double pos[3], double eventOrientation[3])
{
	vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();

	if(cellPicker->Pick3DRay(pos, eventOrientation, m_renderer) >= 0)
	{
		vtkIdType regionId = dynamic_cast<vtkIdTypeArray*>(glyph3D->GetOutput()->GetPointData()->GetArray("InputPointIds"))
			->GetValue(cellPicker->GetPointId());

		return regionId;
	}
	return -1;
}

void iAVR3DObjectVis::setOctree(iAVROctree* octree)
{
	m_octree = octree;
}

vtkDataSet * iAVR3DObjectVis::getDataSet()
{
	return m_dataSet;
}

vtkActor * iAVR3DObjectVis::getActor()
{
	return m_actor;
}

//! This Method iterates through all leaf regions of the octree and stores its center point and it region iD (field data) in an vtkPolyData
void iAVR3DObjectVis::calculateStartPoints()
{
	vtkSmartPointer<vtkPoints> cubeStartPoints = vtkSmartPointer<vtkPoints>::New();
	m_cubePolyData = vtkSmartPointer<vtkPolyData>::New();

	int leafNodes = m_octree->getOctree()->GetNumberOfLeafNodes();
	
	for(int i=0; i< leafNodes; i++)
	{
		double centerPoint[3];
		m_octree->calculateOctreeRegionCenterPos(i, centerPoint);

		cubeStartPoints->InsertNextPoint(centerPoint[0], centerPoint[1], centerPoint[2]);
	}

	m_cubePolyData->SetPoints(cubeStartPoints);
}




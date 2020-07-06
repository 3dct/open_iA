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
#include "vtkMatrix4x4.h"
#include "vtkVertexGlyphFilter.h"

#include <iAConsole.h>
#include <iAvec3.h>
#include <math.h>

iAVR3DObjectVis::iAVR3DObjectVis(vtkRenderer* ren):m_renderer(ren),m_actor(vtkSmartPointer<vtkActor>::New())
{
	m_visible = false;
	defaultActorSize[0] = 0.15;
	defaultActorSize[1] = 0.15;
	defaultActorSize[2] = 0.15;
	currentColorArr = vtkSmartPointer<vtkUnsignedCharArray>::New();
	currentColorArr->SetName("colors");
	currentColorArr->SetNumberOfComponents(4);

	defaultColor = QColor(0, 0, 200, 255);
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

//! Creates for every region of the octree a cube glyph. The cubes are stored in one actor.
void iAVR3DObjectVis::createModelInMiniature()
{
	
	//RESET TO DEFAULT VALUES
	if (m_actor->GetUserMatrix() != NULL)
		m_actor->GetUserMatrix()->Identity();
	m_actor->GetMatrix()->Identity();
	m_actor->SetOrientation(0, 0, 0);
	m_actor->SetScale(1,1,1);
	m_actor->SetPosition(0, 0, 0);
	m_actor->SetOrigin(0, 0, 0);
	

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
	m_actor->Modified();
	m_actor->GetProperty()->SetColor(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF());
	m_actor->SetScale(defaultActorSize);
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

//! Sets the color (rgba) of one cube in the Miniature Model. The other colors stay the same.
//! The region ID of the octree is used
void iAVR3DObjectVis::setCubeColor(QColor col, int regionID)
{
	unsigned char rgb[4] = {col.red(), col.green(), col.blue(), col.alpha()};

	currentColorArr->SetTuple4(regionID, rgb[0], rgb[1], rgb[2], rgb[3]);

	m_cubePolyData->GetCellData()->SetScalars(currentColorArr); //If GlyphMapper Cell Data is needed!
	m_cubePolyData->Modified();
	glyph3D->Update();
}

//! Colors the whole miniature model with the given vector of rgba values ( between 0.0 and 1.0)
//! Resets the current color of all cubes with the new colors!
void iAVR3DObjectVis::applyHeatmapColoring(std::vector<QColor>* colorPerRegion)
{
	currentColorArr = vtkSmartPointer<vtkUnsignedCharArray>::New();
	currentColorArr->SetName("colors");
	currentColorArr->SetNumberOfComponents(4);

	for (int i = 0; i < colorPerRegion->size(); i++)
	{
		currentColorArr->InsertNextTuple4(colorPerRegion->at(i).red(), colorPerRegion->at(i).green(), colorPerRegion->at(i).blue(), colorPerRegion->at(i).alpha());
	}

	m_cubePolyData->GetPointData()->SetScalars(currentColorArr);
	m_cubePolyData->Modified();
	glyph3D->Update();
}

//! This Method calculates the direction from the MiM center to its single cubes and shifts the cubes from the center away
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVR3DObjectVis::applyLinearCubeOffset(double offset)
{
	if(m_cubePolyData == nullptr)
	{
		DEBUG_LOG(QString("No Points to apply offset"));
		return;
	}
	
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);
	
	for (int i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		normDirection.normalize();
		
		iAVec3d move = normDirection * offset;
		iAVec3d newPoint = currentPoint + move;

		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->Modified();
}

//! This Method calculates the direction from the MiM center to its single cubes and shifts the cubes from the center away
//! The shifts is scaled non-linear by the length from the center to the cube
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVR3DObjectVis::applyRelativeCubeOffset(double offset)
{
	if (m_cubePolyData == nullptr)
	{
		DEBUG_LOG(QString("No Points to apply offset"));
		return;
	}

	double maxLength = 0;
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	// Get max length
	for (int i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d direction = currentPoint - centerPos;
		double length = direction.length();

		if (length > maxLength) maxLength = length;
	}

	for (int i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		double currentLength = normDirection.length();
		normDirection.normalize();

		iAVec3d move = normDirection * offset * (currentLength/maxLength);
		iAVec3d newPoint = currentPoint + move;

		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->Modified();
}

//! This Method calculates a position depending shift of the cubes from the MiM center outwards.
//! The cubes are moved in its 4 direction from the center (left, right, up, down).
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVR3DObjectVis::apply4RegionCubeOffset(double offset)
{
	if (m_cubePolyData == nullptr)
	{
		DEBUG_LOG(QString("No Points to apply offset"));
		return;
	}

	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);
	iAVec3d newPoint;

	for (int i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		newPoint = currentPoint;

		// X
		if(currentPoint[0] < centerPos[0])
		{
			newPoint[0] = currentPoint[0] - offset;
		}
		if (currentPoint[0] > centerPos[0])
		{
			newPoint[0] = currentPoint[0] + offset;
		}
		// Y
		if (currentPoint[1] < centerPos[1])
		{
			newPoint[1] = currentPoint[1] - offset;
		}
		if (currentPoint[1] > centerPos[1])
		{
			newPoint[1] = currentPoint[1] + offset;
		}
		// Z
		if (currentPoint[2] < centerPos[2])
		{
			newPoint[2] = currentPoint[2] - offset;
		}
		if (currentPoint[2] > centerPos[2])
		{
			newPoint[2] = currentPoint[2] + offset;
		}
		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->Modified();
}

//! This Method returns the closest cell of the MiM which gets intersected by a ray  
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

vtkSmartPointer<vtkPolyData> iAVR3DObjectVis::getDataSet()
{
	return m_cubePolyData;
}

vtkActor * iAVR3DObjectVis::getActor()
{
	return m_actor;
}

//! This Method iterates through all leaf regions of the octree and stores its center point in an vtkPolyData
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

//! Test method inserts colored point at given Position
void iAVR3DObjectVis::drawPoint(std::vector<double*>* pos, QColor color)
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


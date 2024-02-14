// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRCubicVis.h"

#include <iALog.h>

//#include "iAVR3DText.h"

#include <vtkCellPicker.h>
#include <vtkCubeSource.h>
#include <vtkIdTypeArray.h>
#include <vtkMatrix4x4.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkVertexGlyphFilter.h>

iAVRCubicVis::iAVRCubicVis(vtkRenderer* ren) :m_renderer(ren), m_actor(vtkSmartPointer<vtkActor>::New()), m_activeActor(vtkSmartPointer<vtkActor>::New())
{
	defaultColor = QColor(0, 0, 200, 255);

	activeRegions = std::vector<vtkIdType>();
	activeColors = std::vector<QColor>();

	defaultActorSize[0] = 1;
	defaultActorSize[1] = 1;
	defaultActorSize[2] = 1;

	m_visible = false;
	m_highlightVisible = false;

	glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	glyphColor->SetName("colors");
	glyphColor->SetNumberOfComponents(4);

	glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScales->SetName("scales");
	glyphScales->SetNumberOfComponents(3);
}

void iAVRCubicVis::setOctree(iAVROctree* octree)
{
	m_octree = octree;
}

void iAVRCubicVis::createCubeModel()
{
	//RESET TO DEFAULT VALUES
	if (m_actor->GetUserMatrix() != NULL)
		m_actor->GetUserMatrix()->Identity();
	m_actor->GetMatrix()->Identity();
	m_actor->SetOrientation(0, 0, 0);
	m_actor->SetScale(1, 1, 1);
	m_actor->SetPosition(0, 0, 0);
	m_actor->SetOrigin(0, 0, 0);

	int leafNodes = m_octree->getOctree()->GetNumberOfLeafNodes();
	if (leafNodes <= 0)
	{
		LOG(lvlDebug, QString("The Octree has no leaf nodes!"));
		return;
	}

	calculateStartPoints();

	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();

	glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	glyph3D->GeneratePointIdsOn();
	glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	glyph3D->SetInputData(m_cubePolyData);
	glyph3D->SetScaleModeToScaleByScalar();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(glyph3D->GetOutputPort());

	m_actor->SetMapper(glyphMapper);
	m_actor->GetProperty()->SetColor(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF());
}

void iAVRCubicVis::show()
{
	if (m_visible)
	{
		return;
	}
	m_renderer->AddActor(m_actor);
	m_visible = true;
}

void iAVRCubicVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_renderer->RemoveActor(m_actor);
	m_visible = false;
}

//! Sets the fiber coverage data, which is a vector for every octree level and each region, in which every fiber is stored with its coverage in that particular region.
void iAVRCubicVis::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

//! Returns the Actor for the glyphs
vtkSmartPointer<vtkActor> iAVRCubicVis::getActor()
{
	return m_actor;
}

//! Returns the vtkPolyData for the center points of the glyphs
vtkSmartPointer<vtkPolyData> iAVRCubicVis::getDataSet()
{
	return m_cubePolyData;
}

//! This Method returns the closest cell of the Cube which gets intersected by a ray
vtkIdType iAVRCubicVis::getClosestCellID(double pos[3], double eventOrientation[3])
{
	vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
	cellPicker->AddPickList(m_actor);
	cellPicker->PickFromListOn();

	if (cellPicker->Pick3DRay(pos, eventOrientation, m_renderer) >= 0)
	{
		if (!glyph3D)
		{
			LOG(lvlDebug, "Glyph not set (yet)!");
			return -1;
		}
		auto inputPointIDs = dynamic_cast<vtkIdTypeArray*>(glyph3D->GetOutput()->GetPointData()->GetArray("InputPointIds"));
		if (!inputPointIDs)
		{
			LOG(lvlDebug, "Input point IDs not set!");
			return -1;
		}
		vtkIdType regionId = inputPointIDs->GetValue(cellPicker->GetPointId());

		return regionId;
	}
	return -1;
}

//! Sets the color (rgba) of one cube in the Miniature Model. The other colors stay the same.
//! The region ID of the octree is used
void iAVRCubicVis::setCubeColor(QColor col, int regionID)
{
	unsigned char rgb[4] = { static_cast<unsigned char>(col.red()), static_cast<unsigned char>(col.green()), static_cast<unsigned char>(col.blue()), static_cast<unsigned char>(col.alpha()) };

	glyphColor->SetTuple4(regionID, rgb[0], rgb[1], rgb[2], rgb[3]);

	m_cubePolyData->GetPointData()->AddArray(glyphColor);
}

//! Colors the whole miniature model with the given vector of rgba values ( between 0.0 and 1.0)
//! Resets the current color of all cubes with the new colors!
void iAVRCubicVis::applyHeatmapColoring(std::vector<QColor>* colorPerRegion)
{
	//Remove possible highlights
	removeHighlightedGlyphs();

	glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	glyphColor->SetName("colors");
	glyphColor->SetNumberOfComponents(4);

	for (size_t i = 0; i < colorPerRegion->size(); i++)
	{
		glyphColor->InsertNextTuple4(colorPerRegion->at(i).red(), colorPerRegion->at(i).green(), colorPerRegion->at(i).blue(), colorPerRegion->at(i).alpha());
	}

	m_cubePolyData->GetPointData()->AddArray(glyphColor);
}

//! Creates colored border around the given Cubes. If no/empty color vector or too few colors are given the additional border are black
void iAVRCubicVis::highlightGlyphs(std::vector<vtkIdType>* regionIDs, std::vector<QColor>* colorPerRegion)
{
	if (!regionIDs->empty())
	{
		//Add black color if too few colors are given
		if (regionIDs->size() > colorPerRegion->size())
		{
			for (size_t i = 0; i < colorPerRegion->size() - regionIDs->size(); i++)
			{
				colorPerRegion->push_back(QColor(0, 0, 0));
			}
		}

		activeRegions = *regionIDs;
		activeColors = *colorPerRegion;

		vtkSmartPointer<vtkPolyData> activeData = vtkSmartPointer<vtkPolyData>::New();
		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		activeGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();

		vtkSmartPointer<vtkUnsignedCharArray> activeGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
		activeGlyphColor->SetName("highlightColor");
		activeGlyphColor->SetNumberOfComponents(3);

		vtkSmartPointer<vtkDoubleArray> activeGlyphScales = vtkSmartPointer<vtkDoubleArray>::New();
		activeGlyphScales->SetName("scales");
		activeGlyphScales->SetNumberOfComponents(3);

		for (size_t i = 0; i < regionIDs->size(); i++)
		{
			int iD = regionIDs->at(i);
			points->InsertNextPoint(m_cubePolyData->GetPoint(iD));

			double* regionSize = glyphScales->GetTuple3(iD);
			activeGlyphScales->InsertNextTuple3(regionSize[0], regionSize[1], regionSize[2]);
			activeGlyphColor->InsertNextTuple3(colorPerRegion->at(i).red(), colorPerRegion->at(i).green(), colorPerRegion->at(i).blue());
		}
		activeData->SetPoints(points);
		activeData->GetPointData()->SetScalars(activeGlyphScales);
		activeData->GetPointData()->AddArray(activeGlyphColor);

		vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();

		activeGlyph3D->SetSourceConnection(cubeSource->GetOutputPort());
		activeGlyph3D->SetInputData(activeData);
		activeGlyph3D->SetScaleModeToScaleByScalar();

		// Create a mapper and actor
		vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		glyphMapper->SetInputConnection(activeGlyph3D->GetOutputPort());

		m_activeActor->SetMapper(glyphMapper);
		m_activeActor->GetMapper()->ScalarVisibilityOn();//Use scalars for color
		m_activeActor->GetMapper()->SetScalarModeToUsePointFieldData();
		m_activeActor->GetMapper()->SelectColorArray("highlightColor");
		m_activeActor->GetProperty()->SetRepresentationToWireframe();
		m_activeActor->GetProperty()->SetRenderLinesAsTubes(true);
		m_activeActor->GetProperty()->SetLineWidth(12);
		m_activeActor->SetScale(m_actor->GetScale());
		m_activeActor->SetPosition(m_actor->GetPosition());
		m_activeActor->PickableOff();
		m_activeActor->Modified();

		m_renderer->AddActor(m_activeActor);
		m_highlightVisible = true;
	}
}

//! Removes the colored border around selected Cubes.
void iAVRCubicVis::removeHighlightedGlyphs()
{
	if (!m_highlightVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_activeActor);
	activeRegions.clear();
	activeColors.clear();
	m_highlightVisible = false;
}

//! Redraw the borders of the last selection in black color
void iAVRCubicVis::redrawHighlightedGlyphs()
{
	highlightGlyphs(&activeRegions, &activeColors);
}

double* iAVRCubicVis::getDefaultActorSize()
{
	return defaultActorSize;
}

//! This Method iterates through all leaf regions of the octree and stores its center point in an vtkPolyData
//! It also calculates the region size and adds the scalar array for it
void iAVRCubicVis::calculateStartPoints()
{
	//int count = 0;
	vtkSmartPointer<vtkPoints> cubeStartPoints = vtkSmartPointer<vtkPoints>::New();
	m_cubePolyData = vtkSmartPointer<vtkPolyData>::New();

	glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	glyphScales->SetName("scales");
	glyphScales->SetNumberOfComponents(3);

	int leafNodes = m_octree->getOctree()->GetNumberOfLeafNodes();

	for (int i = 0; i < leafNodes; i++)
	{
		double centerPoint[3];
		m_octree->calculateOctreeRegionCenterPos(i, centerPoint);

		//iAVR3DText text = iAVR3DText(m_renderer);
		//text.create3DLabel(QString("Region %1").arg(i));
		//text.setLabelPos(centerPoint);
		//if(m_octree->getLevel() == 2) text.show();

		//If regions have no coverage resize this 'empty' cube to zero
		if (!m_fiberCoverage->at(m_octree->getLevel()).at(i)->empty()) {
			double regionSize[3];
			m_octree->calculateOctreeRegionSize(i, regionSize);
			glyphScales->InsertNextTuple3(regionSize[0], regionSize[1], regionSize[2]);
			//count++;
		}
		else
		{
			glyphScales->InsertNextTuple3(0, 0, 0);
		}
		cubeStartPoints->InsertNextPoint(centerPoint[0], centerPoint[1], centerPoint[2]);
	}

	m_cubePolyData->SetPoints(cubeStartPoints);
	m_cubePolyData->GetPointData()->SetScalars(glyphScales);
}

//! Applies a linear shift: All regions are displaced by the same factor, regardless of their 
//! distance from the center of the fiber model. This Method calculates the direction from 
//! the center to its single cubes and shifts the cubes linear from the center away
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVRCubicVis::applyRadialDisplacement(double offset)
{
	if (m_cubePolyData == nullptr)
	{
		LOG(lvlDebug, QString("No Points to apply offset"));
		return;
	}

	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (vtkIdType i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		normDirection.normalize();

		iAVec3d move = normDirection * offset;
		iAVec3d newPoint = currentPoint + move;

		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

//! The structure preserving displacement (SP) increases the distance to the center 
//! and the regions, but the relative distances between the regions remain the same.
//! The shifts is scaled relative to the maximal length from the center to one cube and 
//! shifts all cubes from the center away
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVRCubicVis::applySPDisplacement(double offset)
{
	if (m_cubePolyData == nullptr)
	{
		LOG(lvlDebug, QString("No Points to apply offset"));
		return;
	}

	double maxLength = 0;
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	// Get max length
	for (vtkIdType i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d direction = currentPoint - centerPos;
		double length = direction.length();

		if (length > maxLength) maxLength = length;
	}

	for (vtkIdType i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		double currentLength = normDirection.length();
		normDirection.normalize();

		iAVec3d move = normDirection * offset * (currentLength / maxLength);
		iAVec3d newPoint = currentPoint + move;

		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

//! The octant displacement shifts the regions to the nearest octant out of 8 possible octants.
//! The center of the fiber model is the origin of the three - dimensional Euclidean coordinate 
//! system, which forms eight octants through the three axial planes X, Y, and Z.
//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
void iAVRCubicVis::applyOctantDisplacement(double offset)
{
	if (m_cubePolyData == nullptr)
	{
		LOG(lvlDebug, QString("No Points to apply offset"));
		return;
	}

	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);
	iAVec3d newPoint;

	for (vtkIdType i = 0; i < glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(i));
		newPoint = currentPoint;

		// X
		if (currentPoint[0] < centerPos[0])
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
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

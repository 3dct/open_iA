// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRCubicVis.h"

#include <iALog.h>

#include <vtkActor.h>
#include <vtkCellPicker.h>
#include <vtkCubeSource.h>
#include <vtkDoubleArray.h>
#include <vtkGlyph3D.h>
#include <vtkIdTypeArray.h>
#include <vtkMatrix4x4.h>
#include <vtkOctreePointLocator.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iAVRCubicVis::iAVRCubicVis(vtkRenderer* ren) :m_renderer(ren), m_actor(vtkSmartPointer<vtkActor>::New()), m_activeActor(vtkSmartPointer<vtkActor>::New())
{
	m_defaultColor = QColor(0, 0, 200, 255);
	m_activeRegions = std::vector<vtkIdType>();
	m_activeColors = std::vector<QColor>();

	m_defaultActorSize[0] = 1;
	m_defaultActorSize[1] = 1;
	m_defaultActorSize[2] = 1;

	m_visible = false;
	m_highlightVisible = false;

	m_glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_glyphColor->SetName("colors");
	m_glyphColor->SetNumberOfComponents(4);

	m_glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	m_glyphScales->SetName("scales");
	m_glyphScales->SetNumberOfComponents(3);
}

void iAVRCubicVis::setOctree(iAVROctree* octree)
{
	m_octree = octree;
}

void iAVRCubicVis::createCubeModel()
{
	//RESET TO DEFAULT VALUES
	if (m_actor->GetUserMatrix())
	{
		m_actor->GetUserMatrix()->Identity();
	}
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

	m_glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	m_glyph3D->GeneratePointIdsOn();
	m_glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	m_glyph3D->SetInputData(m_cubePolyData);
	m_glyph3D->SetScaleModeToScaleByScalar();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(m_glyph3D->GetOutputPort());

	m_actor->SetMapper(glyphMapper);
	m_actor->GetProperty()->SetColor(m_defaultColor.redF(), m_defaultColor.greenF(), m_defaultColor.blueF());
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

void iAVRCubicVis::setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * fiberCoverage)
{
	m_fiberCoverage = fiberCoverage;
}

vtkSmartPointer<vtkActor> iAVRCubicVis::getActor()
{
	return m_actor;
}

vtkSmartPointer<vtkPolyData> iAVRCubicVis::getDataSet()
{
	return m_cubePolyData;
}

vtkIdType iAVRCubicVis::getClosestCellID(double pos[3], double eventOrientation[3])
{
	vtkSmartPointer<vtkCellPicker> cellPicker = vtkSmartPointer<vtkCellPicker>::New();
	cellPicker->AddPickList(m_actor);
	cellPicker->PickFromListOn();

	if (cellPicker->Pick3DRay(pos, eventOrientation, m_renderer) >= 0)
	{
		if (!m_glyph3D)
		{
			LOG(lvlDebug, "Glyph not set (yet)!");
			return -1;
		}
		auto inputPointIDs = dynamic_cast<vtkIdTypeArray*>(m_glyph3D->GetOutput()->GetPointData()->GetArray("InputPointIds"));
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

void iAVRCubicVis::setCubeColor(QColor col, int regionID)
{
	unsigned char rgb[4] = { static_cast<unsigned char>(col.red()), static_cast<unsigned char>(col.green()), static_cast<unsigned char>(col.blue()), static_cast<unsigned char>(col.alpha()) };

	m_glyphColor->SetTuple4(regionID, rgb[0], rgb[1], rgb[2], rgb[3]);

	m_cubePolyData->GetPointData()->AddArray(m_glyphColor);
}

void iAVRCubicVis::applyHeatmapColoring(std::vector<QColor> const & colorPerRegion)
{
	//Remove possible highlights
	removeHighlightedGlyphs();

	m_glyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_glyphColor->SetName("colors");
	m_glyphColor->SetNumberOfComponents(4);

	for (size_t i = 0; i < colorPerRegion.size(); i++)
	{
		m_glyphColor->InsertNextTuple4(colorPerRegion.at(i).red(), colorPerRegion.at(i).green(), colorPerRegion.at(i).blue(), colorPerRegion.at(i).alpha());
	}

	m_cubePolyData->GetPointData()->AddArray(m_glyphColor);
}

void iAVRCubicVis::highlightGlyphs(std::vector<vtkIdType> const & regionIDs, std::vector<QColor> colorPerRegion)
{
	if (!regionIDs.empty())
	{
		//Add black color if too few colors are given
		if (regionIDs.size() > colorPerRegion.size())
		{
			for (size_t i = 0; i < colorPerRegion.size() - regionIDs.size(); i++)
			{
				colorPerRegion.push_back(QColor(0, 0, 0));
			}
		}

		m_activeRegions = regionIDs;
		m_activeColors = colorPerRegion;

		vtkSmartPointer<vtkPolyData> activeData = vtkSmartPointer<vtkPolyData>::New();
		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		m_activeGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();

		vtkSmartPointer<vtkUnsignedCharArray> activeGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
		activeGlyphColor->SetName("highlightColor");
		activeGlyphColor->SetNumberOfComponents(3);

		vtkSmartPointer<vtkDoubleArray> activeGlyphScales = vtkSmartPointer<vtkDoubleArray>::New();
		activeGlyphScales->SetName("scales");
		activeGlyphScales->SetNumberOfComponents(3);

		for (size_t i = 0; i < regionIDs.size(); i++)
		{
			int iD = regionIDs.at(i);
			points->InsertNextPoint(m_cubePolyData->GetPoint(iD));

			double* regionSize = m_glyphScales->GetTuple3(iD);
			activeGlyphScales->InsertNextTuple3(regionSize[0], regionSize[1], regionSize[2]);
			activeGlyphColor->InsertNextTuple3(colorPerRegion.at(i).red(), colorPerRegion.at(i).green(), colorPerRegion.at(i).blue());
		}
		activeData->SetPoints(points);
		activeData->GetPointData()->SetScalars(activeGlyphScales);
		activeData->GetPointData()->AddArray(activeGlyphColor);

		vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();

		m_activeGlyph3D->SetSourceConnection(cubeSource->GetOutputPort());
		m_activeGlyph3D->SetInputData(activeData);
		m_activeGlyph3D->SetScaleModeToScaleByScalar();

		// Create a mapper and actor
		vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		glyphMapper->SetInputConnection(m_activeGlyph3D->GetOutputPort());

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

void iAVRCubicVis::removeHighlightedGlyphs()
{
	if (!m_highlightVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_activeActor);
	m_activeRegions.clear();
	m_activeColors.clear();
	m_highlightVisible = false;
}

void iAVRCubicVis::redrawHighlightedGlyphs()
{
	highlightGlyphs(m_activeRegions, m_activeColors);
}

double* iAVRCubicVis::getDefaultActorSize()
{
	return m_defaultActorSize;
}

void iAVRCubicVis::calculateStartPoints()
{
	//int count = 0;
	vtkSmartPointer<vtkPoints> cubeStartPoints = vtkSmartPointer<vtkPoints>::New();
	m_cubePolyData = vtkSmartPointer<vtkPolyData>::New();

	m_glyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	m_glyphScales->SetName("scales");
	m_glyphScales->SetNumberOfComponents(3);

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
			m_glyphScales->InsertNextTuple3(regionSize[0], regionSize[1], regionSize[2]);
			//count++;
		}
		else
		{
			m_glyphScales->InsertNextTuple3(0, 0, 0);
		}
		cubeStartPoints->InsertNextPoint(centerPoint[0], centerPoint[1], centerPoint[2]);
	}

	m_cubePolyData->SetPoints(cubeStartPoints);
	m_cubePolyData->GetPointData()->SetScalars(m_glyphScales);
}

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

	for (vtkIdType i = 0; i < m_glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		normDirection.normalize();

		iAVec3d move = normDirection * offset;
		iAVec3d newPoint = currentPoint + move;

		m_glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

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
	for (vtkIdType i = 0; i < m_glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d direction = currentPoint - centerPos;
		double length = direction.length();

		if (length > maxLength) maxLength = length;
	}

	for (vtkIdType i = 0; i < m_glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(i));
		iAVec3d normDirection = currentPoint - centerPos;
		double currentLength = normDirection.length();
		normDirection.normalize();

		iAVec3d move = normDirection * offset * (currentLength / maxLength);
		iAVec3d newPoint = currentPoint + move;

		m_glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

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

	for (vtkIdType i = 0; i < m_glyph3D->GetPolyDataInput(0)->GetNumberOfPoints(); i++)
	{
		iAVec3d currentPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(i));
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
		m_glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->GetPoints()->GetData()->Modified();
	redrawHighlightedGlyphs();
}

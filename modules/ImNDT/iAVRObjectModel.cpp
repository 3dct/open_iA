// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRObjectModel.h"

#include <iAColoredPolyObjectVis.h>
#include <iAVROctreeMetrics.h>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkGlyph3D.h>
#include <vtkLine.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVariantArray.h>

iAVRObjectModel::iAVRObjectModel(vtkRenderer* ren, iAColoredPolyObjectVis* polyObject):
	iAVRCubicVis{ ren }, m_polyObject(polyObject)
{
	m_defaultColor = QColor(126, 0, 223, 255);
	m_volumeActor = vtkSmartPointer<vtkActor>::New();
	m_RegionLinksActor = vtkSmartPointer<vtkActor>::New();
	m_RegionNodesActor = vtkSmartPointer<vtkActor>::New();
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	m_nodeGlyphResetColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_initialPoints = vtkSmartPointer<vtkPoints>::New();

	m_volumeVisible = false;
	m_regionLinksVisible = false;
	m_regionLinkDrawRadius = 0.5;

	// Initial Volume
	// Copy of m_polyObject's data
	m_initialPoints->DeepCopy(polyObject->finalPolyData()->GetPoints());

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(m_polyObject->finalPolyData());
	mapper->SetScalarModeToUsePointFieldData();
	mapper->ScalarVisibilityOn();
	mapper->SelectColorArray("Colors");
	m_volumeActor->SetMapper(mapper);
}

//Resets to the initial volume
void iAVRObjectModel::resetVolume()
{
	m_polyObject->finalPolyData()->GetPoints()->DeepCopy(m_initialPoints);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(m_polyObject->finalPolyData());
	mapper->SetScalarModeToUsePointFieldData();
	mapper->ScalarVisibilityOn();
	mapper->SelectColorArray("Colors");
	m_volumeActor->SetMapper(mapper);
}

void iAVRObjectModel::showVolume()
{
	if (m_volumeVisible)
	{
		return;
	}
	m_renderer->AddActor(m_volumeActor);
	m_volumeVisible = true;
}

void iAVRObjectModel::hideVolume()
{
	if (!m_volumeVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_volumeActor);
	m_volumeVisible = false;
}

void iAVRObjectModel::showRegionLinks()
{
	if (m_regionLinksVisible)
	{
		return;
	}
	m_renderer->AddActor(m_RegionLinksActor);
	m_renderer->AddActor(m_RegionNodesActor);
	m_regionLinksVisible = true;
}

void iAVRObjectModel::hideRegionLinks()
{
	if (!m_regionLinksVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_RegionLinksActor);
	m_renderer->RemoveActor(m_RegionNodesActor);
	m_regionLinksVisible = false;
}

vtkSmartPointer<vtkActor> iAVRObjectModel::getVolumeActor()
{
	return m_volumeActor;
}

double* iAVRObjectModel::getCubePos(int region)
{
	return m_cubePolyData->GetPoint(region);
}

double iAVRObjectModel::getCubeSize(int region)
{
	return m_nodeGlyphScales->GetTuple3(region)[0];
}

void iAVRObjectModel::setNodeColor(std::vector<vtkIdType> const & regions, std::vector<QColor> const & color)
{
	if (m_nodeGlyphResetColor->GetNumberOfTuples() >0)
	{
		for (size_t i = 0; i < regions.size(); i++)
		{
			m_nodeGlyphColor->SetTuple3(regions.at(i), color.at(i).red(), color.at(i).green(), color.at(i).blue());
			m_nodeGlyph3D->Modified();
		}
	}
}

void iAVRObjectModel::resetNodeColor()
{
	if (m_nodeGlyphResetColor->GetNumberOfTuples() > 0)
	{
		m_nodeGlyphColor->DeepCopy(m_nodeGlyphResetColor);
		m_nodeGlyph3D->Modified();
	}
}

iAColoredPolyObjectVis* iAVRObjectModel::getPolyObject()
{
	return m_polyObject;
}

void iAVRObjectModel::createCubeModel()
{
	iAVRCubicVis::createCubeModel();

	//m_actor->GetMapper()->SetScalarVisibility(false);
	m_actor->GetMapper()->ScalarVisibilityOn();
	m_actor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_actor->GetMapper()->SelectColorArray("colors");

	std::vector<QColor> color(m_octree->getNumberOfLeafNodes(), m_defaultColor);
	applyHeatmapColoring(color);

	m_actor->GetProperty()->SetColor(m_defaultColor.redF(), m_defaultColor.greenF(), m_defaultColor.blueF());
	m_actor->GetProperty()->SetRepresentationToWireframe();
	m_actor->GetProperty()->SetRenderLinesAsTubes(true);
	m_actor->GetProperty()->SetLineWidth(2);
	m_actor->Modified();

}

void iAVRObjectModel::renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem)
{
	m_polyObject->renderSelection(sortedSelInds, classID, classColor, activeClassItem);
}

void iAVRObjectModel::moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>> const & m_maxCoverage, double offset, bool relativeMovement)
{
	double maxLength = 0; // m_octree->getMaxDistanceOctCenterToRegionCenter();// m_octree->getMaxDistanceOctCenterToFiber();
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if(relativeMovement)
	{
		for (vtkIdType region = 0; region < m_octree->getNumberOfLeafNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}

	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafNodes(); region++)
	{
		iAVec3d regionCenterPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(region));

		for (auto fiberID : m_maxCoverage.at(m_octree->getLevel()).at(region))
		{
			auto endPointID = m_polyObject->finalObjectStartPointIdx(fiberID) + m_polyObject->finalObjectPointCount(fiberID);

			for (auto pointID = m_polyObject->finalObjectStartPointIdx(fiberID); pointID < endPointID; ++pointID)
			{
				iAVec3d currentPoint = iAVec3d(m_polyObject->finalPolyData()->GetPoint(pointID));
				iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
				iAVec3d normDirection = currentRegionCenterPoint - centerPos;
				double currentLength = normDirection.length();
				normDirection.normalize();
				iAVec3d move = normDirection * offset * ( (relativeMovement)? (currentLength / maxLength) : 1 );
				iAVec3d newPoint = currentPoint + move;
				m_polyObject->finalPolyData()->GetPoints()->SetPoint(pointID, newPoint.data());
			}
		}
	}

	m_polyObject->finalPolyData()->GetPoints()->GetData()->Modified();
}

void iAVRObjectModel::moveFibersbyAllCoveredRegions(double offset, bool relativeMovement)
{
	double maxLength = 0;
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if (relativeMovement)
	{
		for (vtkIdType region = 0; region < m_octree->getNumberOfLeafNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}

	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafNodes(); region++)
	{
		for (auto const & element : *m_fiberCoverage->at(m_octree->getLevel()).at(region))
		{
			iAVec3d currentPoint = iAVec3d(m_polyObject->finalPolyData()->GetPoint(element.first));

			iAVec3d regionCenterPoint = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d normDirection = regionCenterPoint - centerPos;
			double currentLength = normDirection.length();
			normDirection.normalize();

			//Offset gets smaller with coverage
			iAVec3d move = (relativeMovement)
				? normDirection * offset * element.second * (currentLength / maxLength)
				: normDirection * offset * element.second;
			iAVec3d newPoint = currentPoint + move;

			m_polyObject->finalPolyData()->GetPoints()->SetPoint(element.first, newPoint.data());
		}
	}
	m_polyObject->finalPolyData()->GetPoints()->GetData()->Modified();
}

//! Moves all fibers from the octree center away.
//! The fibers belong to the region in which they have their maximum coverage and are moved based on the octant displacement
void iAVRObjectModel::moveFibersbyOctant(std::vector<std::vector<std::vector<vtkIdType>>> const & m_maxCoverage, double offset)
{
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafNodes(); region++)
	{
		iAVec3d currentRegion = iAVec3d(m_glyph3D->GetPolyDataInput(0)->GetPoint(region));
		iAVec3d move = iAVec3d(0,0,0);

		// X
		if (currentRegion[0] < centerPos[0])
		{
			move[0] = - offset;
		}
		if (currentRegion[0] > centerPos[0])
		{
			move[0] = + offset;
		}
		// Y
		if (currentRegion[1] < centerPos[1])
		{
			move[1] = - offset;
		}
		if (currentRegion[1] > centerPos[1])
		{
			move[1] = + offset;
		}
		// Z
		if (currentRegion[2] < centerPos[2])
		{
			move[2] = - offset;
		}
		if (currentRegion[2] > centerPos[2])
		{
			move[2] = + offset;
		}

		for (auto fiberID : m_maxCoverage.at(m_octree->getLevel()).at(region))
		{
			auto endPointID = m_polyObject->finalObjectStartPointIdx(fiberID) + m_polyObject->finalObjectPointCount(fiberID);
			for (auto pointID = m_polyObject->finalObjectStartPointIdx(fiberID); pointID < endPointID; ++pointID)
			{
				iAVec3d currentPoint = iAVec3d(m_polyObject->finalPolyData()->GetPoint(pointID));
				iAVec3d newPoint = currentPoint + move;
				m_polyObject->finalPolyData()->GetPoints()->SetPoint(pointID, newPoint.data());
			}
		}
	}
	m_polyObject->finalPolyData()->GetPoints()->GetData()->Modified();
}

void iAVRObjectModel::createSimilarityNetwork(std::vector<std::vector<std::vector<double>>> const & similarityMetric, double maxFibersInRegions, double worldSize)
{
	createRegionLinks(similarityMetric, worldSize);
	createRegionNodes(maxFibersInRegions, worldSize);
}

void iAVRObjectModel::createRegionLinks(std::vector<std::vector<std::vector<double>>> const & similarityMetric, double worldSize)
{
	vtkNew<vtkPoints> linePoints;
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	vtkNew<vtkCellArray> lines;

	vtkIdType numbPoints = m_cubePolyData->GetNumberOfPoints();
	auto minRadius = worldSize * 0.00084;
	auto maxRadius = worldSize * 0.007;

	calculateNodeLUT(minRadius, maxRadius, 0);

	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	//tubeRadius->SetNumberOfTuples(m_cubePolyData->GetNumberOfPoints());
	tubeRadius->SetNumberOfComponents(1);
	tubeRadius->SetName("TubeRadius");

	m_linkGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_linkGlyphColor->SetName("linkColor");
	m_linkGlyphColor->SetNumberOfComponents(3);

	vtkIdType pointID = 0;
	double rgb[3] = { 0,0,0 };

	for (vtkIdType i = 0; i < numbPoints; i++)
	{
		double radius = 0.0;
		for (vtkIdType j = i + 1; j < numbPoints; j++)
		{
			radius = similarityMetric.at(m_octree->getLevel()).at(i).at(j);

			if (radius > m_regionLinkDrawRadius)
			{
				linePoints->InsertNextPoint(m_cubePolyData->GetPoint(i));
				linePoints->InsertNextPoint(m_cubePolyData->GetPoint(j));

				vtkNew<vtkLine> l;
				l->GetPointIds()->SetId(0, pointID);
				l->GetPointIds()->SetId(1, pointID+1);
				lines->InsertNextCell(l);

				double lineThicknessLog = iAVROctreeMetrics::histogramNormalizationExpo(radius, minRadius, maxRadius, 0.0, 1.0);
				tubeRadius->InsertNextTuple1(lineThicknessLog);
				tubeRadius->InsertNextTuple1(lineThicknessLog);

				m_lut->GetColor(lineThicknessLog, rgb);
				m_linkGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);
				m_linkGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);

				pointID+= 2;
			}
		}

	}
	m_linePolyData->SetPoints(linePoints);
	m_linePolyData->SetLines(lines);

	m_linePolyData->GetPointData()->AddArray(tubeRadius);
	//m_linePolyData->GetCellData()->AddArray(linkGlyphColor);
	m_linePolyData->GetPointData()->AddArray(m_linkGlyphColor);
	m_linePolyData->GetPointData()->SetActiveScalars("TubeRadius");

	vtkNew<vtkTubeFilter> tubeFilter;
	tubeFilter->SetInputData(m_linePolyData);
	tubeFilter->SetNumberOfSides(8);
	//tubeFilter->SetRadius(0.01);
	tubeFilter->SidesShareVerticesOff();
	tubeFilter->CappingOn();
	//tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
	tubeFilter->SetVaryRadiusToVaryRadiusByAbsoluteScalar();
	//tubeFilter->Update();

	// Create a mapper and actor
	vtkNew<vtkPolyDataMapper> lineMapper;
	lineMapper->SetInputConnection(tubeFilter->GetOutputPort());
	lineMapper->ScalarVisibilityOff();

	m_RegionLinksActor->SetMapper(lineMapper);
	m_RegionLinksActor->SetPickable(false);
	//m_RegionLinksActor->GetProperty()->SetRenderLinesAsTubes(true);
	m_RegionLinksActor->GetMapper()->ScalarVisibilityOn();
	m_RegionLinksActor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_RegionLinksActor->GetMapper()->SelectColorArray("linkColor");
}

void iAVRObjectModel::createRegionNodes(double maxFibersInRegions, double worldSize)
{
	vtkNew<vtkPolyData> regionNodes;
	regionNodes->ShallowCopy(m_cubePolyData);

	auto min = worldSize * 0.01;
	auto max = worldSize * 0.077;
	calculateNodeLUT(min, max, 1);

	m_nodeGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_nodeGlyphColor->SetName("nodeColor");
	m_nodeGlyphColor->SetNumberOfComponents(3);

	m_nodeGlyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	m_nodeGlyphScales->SetName("scales");
	m_nodeGlyphScales->SetNumberOfComponents(3);

	for (vtkIdType p = 0; p < regionNodes->GetNumberOfPoints(); p++)
	{
		double fibersInRegion = (double)(m_fiberCoverage->at(m_octree->getLevel()).at(p)->size());
		double sizeLog = 0;
		double rgb[3] = { 0,0,0 };
		if (fibersInRegion > 0)
		{
			sizeLog = iAVROctreeMetrics::histogramNormalizationExpo(fibersInRegion, min, max, 1, maxFibersInRegions);
			m_lut->GetColor(sizeLog, rgb);
		}
		m_nodeGlyphScales->InsertNextTuple3(sizeLog, sizeLog, sizeLog);
		m_nodeGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);
	}
	m_nodeGlyphResetColor->DeepCopy(m_nodeGlyphColor);

	regionNodes->GetPointData()->SetScalars(m_nodeGlyphScales);
	regionNodes->GetPointData()->AddArray(m_nodeGlyphColor);

	vtkNew<vtkCubeSource> cubeSource;

	m_nodeGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	m_nodeGlyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	m_nodeGlyph3D->SetInputData(regionNodes);
	m_nodeGlyph3D->SetScaleModeToScaleByScalar();

	// Create a mapper and actor
	vtkNew<vtkPolyDataMapper> glyphMapper;
	glyphMapper->SetInputConnection(m_nodeGlyph3D->GetOutputPort());

	m_RegionNodesActor->SetMapper(glyphMapper);
	m_RegionNodesActor->GetMapper()->ScalarVisibilityOn();
	m_RegionNodesActor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_RegionNodesActor->GetMapper()->SelectColorArray("nodeColor");
	//m_RegionNodesActor->GetMapper()->ScalarVisibilityOff();
	//m_RegionNodesActor->GetProperty()->SetColor(0.95, 0.32, 0);
	m_RegionNodesActor->PickableOff();
	m_RegionNodesActor->Modified();
}

void iAVRObjectModel::calculateNodeLUT(double min, double max, int colorScheme)
{
	QColor a;
	QColor b;
	QColor c;
	QColor d;
	QColor e;
	QColor f;

	if(colorScheme == 0)
	{
		a = QColor(255,177,105);
		b = QColor(255,155,60);
		c = QColor(255,123,0);
		d = QColor(214,104,0);
		e = QColor(178,84,0);
		f = QColor(128,62,0);
	}
	else if(colorScheme == 1)
	{
		a = QColor(242, 240, 247);
		b = QColor(218, 218, 235);
		c = QColor(188, 189, 220);
		d = QColor(158, 154, 200);
		e = QColor(117, 107, 177);
		f = QColor(84, 39, 143);
	}

	m_lut = vtkSmartPointer<vtkLookupTable>::New();

	m_lut->SetNumberOfTableValues(6);
	//m_lut->SetScaleToLog10();
	m_lut->Build();

	m_lut->SetTableValue(0, f.redF(), f.greenF(), f.blueF());
	m_lut->SetTableValue(1, e.redF(), e.greenF(), e.blueF());
	m_lut->SetTableValue(2, d.redF(), d.greenF(), d.blueF());
	m_lut->SetTableValue(3, c.redF(), c.greenF(), c.blueF());
	m_lut->SetTableValue(4, b.redF(), b.greenF(), b.blueF());
	m_lut->SetTableValue(5, a.redF(), a.greenF(), a.blueF());

	m_lut->SetTableRange(min, max);
}

void iAVRObjectModel::filterRegionLinks(int sign)
{
	double step = 0.05;

	if(sign > 0) m_regionLinkDrawRadius += step;
	else m_regionLinkDrawRadius -= step;

	if (m_regionLinkDrawRadius < 0) m_regionLinkDrawRadius = 0.95;
	else if (m_regionLinkDrawRadius >= 0.95) m_regionLinkDrawRadius = 0.0;

	if (m_regionLinkDrawRadius - 0.0 <= 0.00001) m_regionLinkDrawRadius = 0.0;
}

double iAVRObjectModel::getJaccardFilterVal() const
{
	return m_regionLinkDrawRadius;
}

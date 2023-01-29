// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRObjectModel.h"

#include <iA3DColoredPolyObjectVis.h>
#include <iALog.h>
#include <iAVROctreeMetrics.h>

#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkLine.h>
#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVariantArray.h>
#include <vtkPointData.h>


iAVRObjectModel::iAVRObjectModel(vtkRenderer* ren, iA3DColoredPolyObjectVis* polyObject, vtkTable* objectTable, iACsvIO io, iACsvConfig csvConfig):
	iAVRCubicVis{ ren }, m_polyObject(polyObject), m_objectTable(objectTable), m_io(io), m_csvConfig(csvConfig)
{
	defaultColor = QColor(126, 0, 223, 255);
	m_volumeActor = vtkSmartPointer<vtkActor>::New();
	m_RegionLinksActor = vtkSmartPointer<vtkActor>::New();
	m_RegionNodesActor = vtkSmartPointer<vtkActor>::New();
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	nodeGlyphResetColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_initialPoints = vtkSmartPointer<vtkPoints>::New();

	m_volumeVisible = false;
	m_regionLinksVisible = false;
	m_regionLinkDrawRadius = 0.5;

	// Initial Volume 
	// Copy of m_polyObject's data 
	// Other way of copying?:  vtkDataSet->CopyData()
	// Like in objectvis\iAvtkTubeFilter.cpp 
	m_initialPoints->DeepCopy(polyObject->finalPolyData()->GetPoints());
	//m_PolyObjectActor = m_polyObject->createPolyActor(m_renderer);
	//m_volumeActor = m_PolyObjectActor->actor();

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
	// vtkSmartPointer<vtkPoints> temp = vtkSmartPointer<vtkPoints>::New();
	// temp->DeepCopy(m_initialPoints);
	// m_polyObject->polyData()->SetPoints(temp);
	m_polyObject->finalPolyData()->GetPoints()->DeepCopy(m_initialPoints);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputData(m_polyObject->finalPolyData());
	mapper->SetScalarModeToUsePointFieldData();
	mapper->ScalarVisibilityOn();
	mapper->SelectColorArray("Colors");
	m_volumeActor->SetMapper(mapper);
	//m_PolyObjectActor->updated();
	//m_PolyObjectActor = m_polyObject->createPolyActor(m_renderer);
	//m_volumeActor = m_PolyObjectActor->actor();
}

void iAVRObjectModel::showVolume()
{
	//m_PolyObjectActor->show();
	
	if (m_volumeVisible)
	{
		return;
	}
	m_renderer->AddActor(m_volumeActor);
	m_volumeVisible = true;
}

void iAVRObjectModel::hideVolume()
{
	//m_PolyObjectActor->hide();
	
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
	//return m_PolyObjectActor->actor();
	return m_volumeActor;
}


double* iAVRObjectModel::getCubePos(int region)
{
	return m_cubePolyData->GetPoint(region);
}

double iAVRObjectModel::getCubeSize(int region)
{
	return nodeGlyphScales->GetTuple3(region)[0];
}

//! Colors the cube nodes with the given region IDs with a given color
//! Both vectors must have equal length
void iAVRObjectModel::setNodeColor(std::vector<vtkIdType> regions, std::vector<QColor> color)
{
	if (nodeGlyphResetColor->GetNumberOfTuples() >0)
	{
		for (size_t i = 0; i < regions.size(); i++)
		{
			nodeGlyphColor->SetTuple3(regions.at(i), color.at(i).red(), color.at(i).green(), color.at(i).blue());
			nodeGlyph3D->Modified();
		}
	}
}

void iAVRObjectModel::resetNodeColor()
{
	if (nodeGlyphResetColor->GetNumberOfTuples() > 0)
	{
		nodeGlyphColor->DeepCopy(nodeGlyphResetColor);
		nodeGlyph3D->Modified();
	}
}

iA3DColoredPolyObjectVis* iAVRObjectModel::getPolyObject()
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

	std::vector<QColor>* color = new std::vector<QColor>(m_octree->getNumberOfLeafeNodes(), defaultColor);
	applyHeatmapColoring(color);
	
	m_actor->GetProperty()->SetColor(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF());
	m_actor->GetProperty()->SetRepresentationToWireframe();
	m_actor->GetProperty()->SetRenderLinesAsTubes(true);
	m_actor->GetProperty()->SetLineWidth(2);
	m_actor->Modified();

}

void iAVRObjectModel::renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem)
{
	m_polyObject->renderSelection(sortedSelInds, classID, classColor, activeClassItem);
}

//! Moves all fibers from the octree center away.
//! The fibers belong to the region in which they have their maximum coverage
//! The flag relativMovement decides if the offset is applied to the relative (radial) octree region postion 
//! or linear (SP)
//! Should only be called if the mappers are set!
void iAVRObjectModel::moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset, bool relativMovement)
{
	double maxLength = 0; // m_octree->getMaxDistanceOctCenterToRegionCenter();// m_octree->getMaxDistanceOctCenterToFiber();
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if(relativMovement)
	{
		for (vtkIdType region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}

	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));

		for (auto fiberID : m_maxCoverage->at(m_octree->getLevel()).at(region))
		{
			auto endPointID = m_polyObject->finalObjectStartPointIdx(fiberID) + m_polyObject->finalObjectPointCount(fiberID);

			for (auto pointID = m_polyObject->finalObjectStartPointIdx(fiberID); pointID < endPointID; ++pointID)
			{
				iAVec3d currentPoint = iAVec3d(m_polyObject->finalPolyData()->GetPoint(pointID));
				iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
				iAVec3d normDirection = currentRegionCenterPoint - centerPos;
				double currentLength = normDirection.length();
				normDirection.normalize();

				iAVec3d move;
				if (relativMovement) move = normDirection * offset * (currentLength / maxLength);
				else move = normDirection * offset;
				iAVec3d newPoint = currentPoint + move;

				m_polyObject->finalPolyData()->GetPoints()->SetPoint(pointID, newPoint.data());
			}
		}
	}
	
	m_polyObject->finalPolyData()->GetPoints()->GetData()->Modified();
}

//! Moves all fibers from the octree center away.
//! The fibers belong to every region in which they have a coverage
//! Should only be called if the mappers are set!
void iAVRObjectModel::moveFibersbyAllCoveredRegions(double offset, bool relativMovement)
{
	double maxLength = 0;
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if(relativMovement)
	{
		for (vtkIdType region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}


	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{

		for (auto element : *m_fiberCoverage->at(m_octree->getLevel()).at(region))
		{

			iAVec3d currentPoint = iAVec3d(m_polyObject->finalPolyData()->GetPoint(element.first));

			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d normDirection = regionCenterPoint - centerPos;
			double currentLength = normDirection.length();
			normDirection.normalize();

			//Offset gets smaller with coverage
			iAVec3d move;
			if (relativMovement) move = normDirection * offset * element.second * (currentLength / maxLength);
			else move = normDirection * offset * element.second;
			iAVec3d newPoint = currentPoint + move;

			m_polyObject->finalPolyData()->GetPoints()->SetPoint(element.first, newPoint.data());

		}
	}
	m_polyObject->finalPolyData()->GetPoints()->GetData()->Modified();
}

//! Moves all fibers from the octree center away.
//! The fibers belong to the region in which they have their maximum coverage and are moved based on the octant displacement
void iAVRObjectModel::moveFibersbyOctant(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset)
{
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (vtkIdType region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		iAVec3d currentRegion = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
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
		
		for (auto fiberID : m_maxCoverage->at(m_octree->getLevel()).at(region))
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

void iAVRObjectModel::createSimilarityNetwork(std::vector<std::vector<std::vector<double>>>* similarityMetric, double maxFibersInRegions, double worldSize)
{
	createRegionLinks(similarityMetric, worldSize);
	createRegionNodes(maxFibersInRegions, worldSize);
}

void iAVRObjectModel::createRegionLinks(std::vector<std::vector<std::vector<double>>>* similarityMetric, double worldSize)
{
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

	vtkIdType numbPoints = m_cubePolyData->GetNumberOfPoints();
	auto minRadius = worldSize * 0.00084;
	auto maxRadius = worldSize * 0.007;

	calculateNodeLUT(minRadius, maxRadius, 0);

	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	//tubeRadius->SetNumberOfTuples(m_cubePolyData->GetNumberOfPoints());
	tubeRadius->SetNumberOfComponents(1);
	tubeRadius->SetName("TubeRadius");

	linkGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	linkGlyphColor->SetName("linkColor");
	linkGlyphColor->SetNumberOfComponents(3);

	vtkIdType pointID = 0;
	double rgb[3] = { 0,0,0 };

	for (vtkIdType i = 0; i < numbPoints; i++)
	{
		double radius = 0.0;
		for (vtkIdType j = i + 1; j < numbPoints; j++)
		{
			radius = similarityMetric->at(m_octree->getLevel()).at(i).at(j);
			
			if (radius > m_regionLinkDrawRadius)
			{
				linePoints->InsertNextPoint(m_cubePolyData->GetPoint(i));
				linePoints->InsertNextPoint(m_cubePolyData->GetPoint(j));

				auto l = vtkSmartPointer<vtkLine>::New();
				l->GetPointIds()->SetId(0, pointID);
				l->GetPointIds()->SetId(1, pointID+1);
				lines->InsertNextCell(l);

				double lineThicknessLog = iAVROctreeMetrics::histogramNormalizationExpo(radius, minRadius, maxRadius, 0.0, 1.0);
				tubeRadius->InsertNextTuple1(lineThicknessLog);
				tubeRadius->InsertNextTuple1(lineThicknessLog);

				m_lut->GetColor(lineThicknessLog, rgb);
				linkGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);
				linkGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);

				pointID+= 2;
			}
		}
		
	}
	m_linePolyData->SetPoints(linePoints);
	m_linePolyData->SetLines(lines);

	m_linePolyData->GetPointData()->AddArray(tubeRadius);
	//m_linePolyData->GetCellData()->AddArray(linkGlyphColor);
	m_linePolyData->GetPointData()->AddArray(linkGlyphColor);
	m_linePolyData->GetPointData()->SetActiveScalars("TubeRadius");

	vtkSmartPointer<vtkTubeFilter> tubeFilter =	vtkSmartPointer<vtkTubeFilter>::New();
	tubeFilter->SetInputData(m_linePolyData);
	tubeFilter->SetNumberOfSides(8);
	//tubeFilter->SetRadius(0.01);
	tubeFilter->SidesShareVerticesOff();
	tubeFilter->CappingOn();
	//tubeFilter->SetVaryRadiusToVaryRadiusByScalar();
	tubeFilter->SetVaryRadiusToVaryRadiusByAbsoluteScalar();
	//tubeFilter->Update();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
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
	vtkSmartPointer<vtkPolyData> regionNodes = vtkSmartPointer<vtkPolyData>::New();
	regionNodes->ShallowCopy(m_cubePolyData);

	auto min = worldSize * 0.01;
	auto max = worldSize * 0.077;
	calculateNodeLUT(min, max, 1);

	nodeGlyphColor = vtkSmartPointer<vtkUnsignedCharArray>::New();
	nodeGlyphColor->SetName("nodeColor");
	nodeGlyphColor->SetNumberOfComponents(3);

	nodeGlyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	nodeGlyphScales->SetName("scales");
	nodeGlyphScales->SetNumberOfComponents(3);

	for (vtkIdType p = 0; p < regionNodes->GetNumberOfPoints(); p++)
	{
		double fibersInRegion = (double)(m_fiberCoverage->at(m_octree->getLevel()).at(p)->size());	
		double sizeLog = 0;
		double rgb[3] = { 0,0,0 };
		if(fibersInRegion > 0)
		{
			sizeLog = iAVROctreeMetrics::histogramNormalizationExpo(fibersInRegion, min, max, 1, maxFibersInRegions);
			m_lut->GetColor(sizeLog, rgb);
		}
		nodeGlyphScales->InsertNextTuple3(sizeLog, sizeLog, sizeLog);
		nodeGlyphColor->InsertNextTuple3(rgb[0] * 255, rgb[1] * 255, rgb[2] * 255);
	}
	nodeGlyphResetColor->DeepCopy(nodeGlyphColor);

	regionNodes->GetPointData()->SetScalars(nodeGlyphScales);
	regionNodes->GetPointData()->AddArray(nodeGlyphColor);

	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	
	nodeGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	nodeGlyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	nodeGlyph3D->SetInputData(regionNodes);
	nodeGlyph3D->SetScaleModeToScaleByScalar();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(nodeGlyph3D->GetOutputPort());

	m_RegionNodesActor->SetMapper(glyphMapper);
	m_RegionNodesActor->GetMapper()->ScalarVisibilityOn();
	m_RegionNodesActor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_RegionNodesActor->GetMapper()->SelectColorArray("nodeColor");
	//m_RegionNodesActor->GetMapper()->ScalarVisibilityOff();
	//m_RegionNodesActor->GetProperty()->SetColor(0.95, 0.32, 0);
	m_RegionNodesActor->PickableOff();
	m_RegionNodesActor->Modified();
}

//! Calculates the LUT for the regionLinks (0) and the regionNodes (1)
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

//! Cycles between values from 0.95 to 0
//! The sign defines if the values are increased/decreased
void iAVRObjectModel::filterRegionLinks(int sign)
{
	double step = 0.05;

	if(sign > 0) m_regionLinkDrawRadius += step;
	else m_regionLinkDrawRadius -= step;

	if (m_regionLinkDrawRadius < 0) m_regionLinkDrawRadius = 0.95;
	else if (m_regionLinkDrawRadius >= 0.95) m_regionLinkDrawRadius = 0.0;

	if (m_regionLinkDrawRadius - 0.0 <= 0.00001) m_regionLinkDrawRadius = 0.0;
}

double iAVRObjectModel::getJaccardFilterVal()
{
	return m_regionLinkDrawRadius;
}

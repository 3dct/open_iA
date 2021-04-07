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
#include "iAVRVolume.h"

#include <iALog.h>
#include <iA3DCylinderObjectVis.h>
#include <iAVRMetrics.h>

#include <vtkVariantArray.h>
#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedCharArray.h>
#include <vtkCleanPolyData.h>
#include <vtkCubeSource.h>

iAVRVolume::iAVRVolume(vtkRenderer* ren, vtkTable* objectTable, iACsvIO io) :m_objectTable(objectTable), m_io(io), iAVRCubicVis{ren}
{
	defaultColor = QColor(126, 0, 223, 255);
	m_volumeActor = vtkSmartPointer<vtkActor>::New();
	m_RegionLinksActor = vtkSmartPointer<vtkActor>::New();
	m_RegionNodesActor = vtkSmartPointer<vtkActor>::New();
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	nodeGlyphResetColor = vtkSmartPointer<vtkUnsignedCharArray>::New();

	m_volumeVisible = false;
	m_regionLinksVisible = false;
	m_regionLinkDrawRadius = 0.5;

	resetVolume();
}

void iAVRVolume::resetVolume()
{
	m_cylinderVis = new iA3DCylinderObjectVis(m_renderer, m_objectTable, m_io.getOutputMapping(), QColor(140, 140, 140, 255), std::map<size_t, std::vector<iAVec3f> >());
	m_volumeActor = m_cylinderVis->getActor();
	//m_volumeActor->AddPosition(1,200,1);
}

void iAVRVolume::showVolume()
{
	if (m_volumeVisible)
	{
		return;
	}
	m_renderer->AddActor(m_volumeActor);
	m_volumeVisible = true;
}

void iAVRVolume::hideVolume()
{
	if (!m_volumeVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_volumeActor);
	m_volumeVisible = false;
}

void iAVRVolume::showRegionLinks()
{
	if (m_regionLinksVisible)
	{
		return;
	}
	m_renderer->AddActor(m_RegionLinksActor);
	m_renderer->AddActor(m_RegionNodesActor);
	m_regionLinksVisible = true;
}

void iAVRVolume::hideRegionLinks()
{
	if (!m_regionLinksVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_RegionLinksActor);
	m_renderer->RemoveActor(m_RegionNodesActor);
	m_regionLinksVisible = false;
}

vtkSmartPointer<vtkActor> iAVRVolume::getVolumeActor()
{
	return m_volumeActor;
}


double* iAVRVolume::getCubePos(int region)
{
	return m_cubePolyData->GetPoint(region);
}

double iAVRVolume::getCubeSize(int region)
{
	return nodeGlyphScales->GetTuple3(region)[0];
}

//! Colors the cube nodes with the given region IDs with a given color
//! Both vectors must have equal length
void iAVRVolume::setNodeColor(std::vector<vtkIdType> regions, std::vector<QColor> color)
{
	if (nodeGlyphResetColor->GetNumberOfTuples() >0)
	{
		for (vtkIdType i = 0; i < regions.size(); i++)
		{
			nodeGlyphColor->SetTuple3(regions.at(i), color.at(i).red(), color.at(i).green(), color.at(i).blue());
			nodeGlyph3D->Modified();
		}
	}
}

void iAVRVolume::resetNodeColor()
{
	if (nodeGlyphResetColor->GetNumberOfTuples() > 0)
	{
		nodeGlyphColor->DeepCopy(nodeGlyphResetColor);
		nodeGlyph3D->Modified();
	}
}

void iAVRVolume::setMappers(std::unordered_map<vtkIdType, vtkIdType> pointIDToCsvIndex, std::unordered_multimap<vtkIdType, vtkIdType> csvIndexToPointID)
{
	m_pointIDToCsvIndex = pointIDToCsvIndex;
	m_csvIndexToPointID = csvIndexToPointID;
}

vtkSmartPointer<vtkPolyData> iAVRVolume::getVolumeData()
{
	return m_cylinderVis->getPolyData();
}

void iAVRVolume::createCubeModel()
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
	m_actor->GetProperty()->SetLineWidth(4);
	m_actor->Modified();

}

void iAVRVolume::renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem)
{
	m_cylinderVis->renderSelection(sortedSelInds, classID, classColor, activeClassItem);
}

//! Moves all fibers from the octree center away.
//! The fibers belong to the region in which they have their maximum coverage
//! The flag relativMovement decides if the offset is applied to the relative octree region postion or linear
//! Should only be called if the mappers are set!
void iAVRVolume::moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset, bool relativMovement)
{
	double maxLength = 0; // m_octree->getMaxDistanceOctCenterToRegionCenter();// m_octree->getMaxDistanceOctCenterToFiber();
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if(relativMovement)
	{
		for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}

	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));

		for (auto fiberID : m_maxCoverage->at(m_octree->getLevel()).at(region))
		{

			auto findKeys = m_csvIndexToPointID.equal_range(fiberID);
			for (auto it = findKeys.first; it != findKeys.second; ++it) 
			{
				iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(it->second));
				iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
				iAVec3d normDirection = currentRegionCenterPoint - centerPos;
				double currentLength = normDirection.length();
				normDirection.normalize();

				iAVec3d move;
				if (relativMovement) move = normDirection * offset * (currentLength / maxLength);
				else move = normDirection * offset;
				iAVec3d newPoint = currentPoint + move;

				m_cylinderVis->getPolyData()->GetPoints()->SetPoint(it->second, newPoint.data());
			}
		}
	}
	
	m_cylinderVis->getPolyData()->GetPoints()->GetData()->Modified();
}

//! Moves all fibers from the octree center away.
//! The fibers belong to every region in which they have a coverage
//! Should only be called if the mappers are set!
void iAVRVolume::moveFibersbyAllCoveredRegions(double offset, bool relativMovement)
{
	double maxLength = 0;
	double centerPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	if(relativMovement)
	{
		for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
		{
			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d direction = regionCenterPoint - centerPos;
			double length = direction.length();
			if (length > maxLength) maxLength = length;		// Get max length
		}
	}


	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{

		for (auto element : *m_fiberCoverage->at(m_octree->getLevel()).at(region))
		{

			iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(element.first));

			iAVec3d regionCenterPoint = iAVec3d(glyph3D->GetPolyDataInput(0)->GetPoint(region));
			iAVec3d normDirection = regionCenterPoint - centerPos;
			double currentLength = normDirection.length();
			normDirection.normalize();

			//Offset gets smaller with coverage
			iAVec3d move;
			if (relativMovement) move = normDirection * offset * element.second * (currentLength / maxLength);
			else move = normDirection * offset * element.second;
			iAVec3d newPoint = currentPoint + move;

			m_cylinderVis->getPolyData()->GetPoints()->SetPoint(element.first, newPoint.data());

		}
	}
	m_cylinderVis->getPolyData()->GetPoints()->GetData()->Modified();
}

void iAVRVolume::moveFibersby4Regions(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset)
{
	double centerPoint[3]{};
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
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
			auto findKeys = m_csvIndexToPointID.equal_range(fiberID);
			for (auto it = findKeys.first; it != findKeys.second; ++it)
			{
				iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(it->second));
				iAVec3d newPoint = currentPoint + move;

				m_cylinderVis->getPolyData()->GetPoints()->SetPoint(it->second, newPoint.data());
			}
		}
		
	}
	m_cylinderVis->getPolyData()->GetPoints()->GetData()->Modified();
}

void iAVRVolume::createRegionLinks(std::vector<std::vector<std::vector<double>>>* similarityMetric, double maxFibersInRegions, double worldSize)
{
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

	int numbPoints = m_cubePolyData->GetNumberOfPoints();
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

	for (int i = 0; i < numbPoints; i++)
	{
		double radius = 0.0;
		for (int j = i + 1; j < numbPoints; j++)
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

				double lineThicknessLog = iAVRMetrics::histogramNormalizationExpo(radius, minRadius, maxRadius, 0.0, 1.0);
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

	createRegionNodes(maxFibersInRegions, worldSize);
}

void iAVRVolume::createRegionNodes(double maxFibersInRegions, double worldSize)
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

	for (int p = 0; p < regionNodes->GetNumberOfPoints(); p++)
	{
		double fibersInRegion = (double)(m_fiberCoverage->at(m_octree->getLevel()).at(p)->size());	
		double sizeLog = 0;
		double rgb[3] = { 0,0,0 };
		if(fibersInRegion > 0)
		{
			sizeLog = iAVRMetrics::histogramNormalizationExpo(fibersInRegion, min, max, 1, maxFibersInRegions);
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
void iAVRVolume::calculateNodeLUT(double min, double max, int colorScheme)
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
void iAVRVolume::filterRegionLinks(int sign)
{
	double step = 0.05;

	if(sign > 0) m_regionLinkDrawRadius += step;
	else m_regionLinkDrawRadius -= step;

	if (m_regionLinkDrawRadius < 0) m_regionLinkDrawRadius = 0.95;
	else if (m_regionLinkDrawRadius >= 0.95) m_regionLinkDrawRadius = 0.0;

	if (m_regionLinkDrawRadius - 0.0 <= 0.00001) m_regionLinkDrawRadius = 0.0;
}

double iAVRVolume::getJaccardFilterVal()
{
	return m_regionLinkDrawRadius;
}

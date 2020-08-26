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

#include <iAConsole.h>
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

iAVRVolume::iAVRVolume(vtkRenderer* ren, vtkTable* objectTable, iACsvIO io) :m_objectTable(objectTable), m_io(io), iAVRCubicRepresentation{ren}
{
	defaultColor = QColor(126, 0, 223, 255);
	m_volumeActor = vtkSmartPointer<vtkActor>::New();
	m_RegionLinksActor = vtkSmartPointer<vtkActor>::New();
	m_RegionNodesActor = vtkSmartPointer<vtkActor>::New();

	m_volumeVisible = false;
	m_regionLinksVisible = false;
	m_regionLinkDrawRadius = 0.9;

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
	iAVRCubicRepresentation::createCubeModel();

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
//! Should only be called if the mappers are set!
void iAVRVolume::moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset)
{
	double maxLength = 0; // m_octree->getMaxDistanceOctCenterToRegionCenter();// m_octree->getMaxDistanceOctCenterToFiber();
	double centerPoint[3];
	double regionCenterPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		m_octree->calculateOctreeRegionCenterPos(region, regionCenterPoint);

		iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
		iAVec3d direction = currentRegionCenterPoint - centerPos;
		double length = direction.length();
		if (length > maxLength) maxLength = length;		// Get max length
	}

	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		//DEBUG_LOG(QString("\n<< REGION %1 >>").arg(region));

		m_octree->calculateOctreeRegionCenterPos(region, regionCenterPoint);

		for (auto fiberID : m_maxCoverage->at(m_octree->getLevel()).at(region))
		{
			//DEBUG_LOG(QString("\n\nFiber <%1> is in Region").arg(element.first));

			auto findKeys = m_csvIndexToPointID.equal_range(fiberID);
			for (auto it = findKeys.first; it != findKeys.second; ++it) {

				//DEBUG_LOG(QString("It has PolyPointID %1").arg(it->second));
				iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(it->second));
				iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
				iAVec3d normDirection = currentRegionCenterPoint - centerPos;
				double currentLength = normDirection.length();
				normDirection.normalize();

				iAVec3d move = normDirection * offset * (currentLength / maxLength);
				iAVec3d newPoint = currentPoint + move;

				m_cylinderVis->getPolyData()->GetPoints()->SetPoint(it->second, newPoint.data());
			}
		}
	}
	
	m_cylinderVis->getPolyData()->GetPoints()->GetData()->Modified();
	//m_octree->getOctree()->Modified();
}

//! Moves all fibers from the octree center away.
//! The fibers belong to every region in which they have a coverage
//! Should only be called if the mappers are set!
void iAVRVolume::moveFibersbyAllCoveredRegions(double offset)
{
	double centerPoint[3];
	double regionCenterPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	//for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	//{
	//	m_octree->calculateOctreeRegionCenterPos(region, regionCenterPoint);

	//	iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
	//	iAVec3d direction = currentRegionCenterPoint - centerPos;
	//	double length = direction.length();
	//	if (length > maxLength) maxLength = length;		// Get max length
	//}

	for (int region = 0; region < m_octree->getNumberOfLeafeNodes(); region++)
	{
		//DEBUG_LOG(QString("\n<< REGION %1 >>").arg(region));

		for (auto element : *m_fiberCoverage->at(m_octree->getLevel()).at(region))
		{

			iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(element.first));

			m_octree->calculateOctreeRegionCenterPos(region, regionCenterPoint);
			iAVec3d currentRegionCenterPoint = iAVec3d(regionCenterPoint);
			iAVec3d normDirection = currentRegionCenterPoint - centerPos;
			double currentLength = normDirection.length();
			normDirection.normalize();

			//Offset gets smaller with coverage
			iAVec3d move = normDirection * offset * element.second;// *(currentLength / maxLength);
			iAVec3d newPoint = currentPoint + move;

			m_cylinderVis->getPolyData()->GetPoints()->SetPoint(element.first, newPoint.data());

		}

	}
	m_cylinderVis->getPolyData()->GetPoints()->GetData()->Modified();
	//m_octree->getOctree()->Modified();
}

void iAVRVolume::createRegionLinks(std::vector<std::vector<std::vector<double>>>* similarityMetric, double maxFibersInRegions)
{
	vtkSmartPointer<vtkPoints> linePoints = vtkSmartPointer<vtkPoints>::New();
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

	int numbPoints = m_cubePolyData->GetNumberOfPoints();

	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	//tubeRadius->SetNumberOfTuples(m_cubePolyData->GetNumberOfPoints());
	tubeRadius->SetNumberOfComponents(1);
	tubeRadius->SetName("TubeRadius");

	/*auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetName("Colors");
	colors->SetNumberOfComponents(3);
	colors->SetNumberOfTuples(m_cubePolyData->GetNumberOfPoints());*/
	vtkIdType pointID = 0;

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

				double lineThicknessLog = iAVRMetrics::histogramNormalizationExpo(radius, 1, 9, 0, 1);
				tubeRadius->InsertNextTuple1(lineThicknessLog);
				tubeRadius->InsertNextTuple1(lineThicknessLog);

				pointID+= 2;
			}
		}
		
	}
	m_linePolyData->SetPoints(linePoints);
	m_linePolyData->SetLines(lines);

	//linePolyData->GetCellData()->AddArray(tubeRadius);
	//linePolyData->GetCellData()->SetActiveScalars("TubeRadius");
	m_linePolyData->GetPointData()->AddArray(tubeRadius);
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
	//m_RegionLinksActor->GetProperty()->SetLineWidth(3);
	m_RegionLinksActor->GetProperty()->SetColor(0.980, 0.607, 0);
	//m_RegionLinksActor->GetProperty()->SetOpacity(0.7);

	createRegionNodes(maxFibersInRegions);
}

void iAVRVolume::createRegionNodes(double maxFibersInRegions)
{
	vtkSmartPointer<vtkPolyData> regionNodes = vtkSmartPointer<vtkPolyData>::New();
	regionNodes->ShallowCopy(m_cubePolyData);
	//vtkSmartPointer<vtkCleanPolyData> cleanPolyData = vtkSmartPointer<vtkCleanPolyData>::New();
	//cleanPolyData->SetInputData(m_linePolyData);
	//cleanPolyData->Update();
	//regionNodes = cleanPolyData->GetOutput();

	nodeGlyphScales = vtkSmartPointer<vtkDoubleArray>::New();
	nodeGlyphScales->SetName("scales");
	nodeGlyphScales->SetNumberOfComponents(3);

	for (int p = 0; p < regionNodes->GetNumberOfPoints(); p++)
	{
		double fibersInRegion = (double)(m_fiberCoverage->at(m_octree->getLevel()).at(p)->size());	
		double sizeLog = 0;
		if(fibersInRegion > 0)
		{
			sizeLog = iAVRMetrics::histogramNormalizationExpo(fibersInRegion, 17, 130, 1, maxFibersInRegions);
		}
		nodeGlyphScales->InsertNextTuple3(sizeLog, sizeLog, sizeLog);
	}

	regionNodes->GetPointData()->SetScalars(nodeGlyphScales);
	//regionNodes->GetPointData()->AddArray(glyphColor);

	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	
	vtkSmartPointer<vtkGlyph3D> nodeGlyph3D = vtkSmartPointer<vtkGlyph3D>::New();
	nodeGlyph3D->SetSourceConnection(cubeSource->GetOutputPort());
	nodeGlyph3D->SetInputData(regionNodes);
	nodeGlyph3D->SetScaleModeToScaleByScalar();

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	glyphMapper->SetInputConnection(nodeGlyph3D->GetOutputPort());

	m_RegionNodesActor->SetMapper(glyphMapper);
	//m_RegionNodesActor->GetMapper()->ScalarVisibilityOn();
	//m_RegionNodesActor->GetMapper()->SetScalarModeToUsePointFieldData();
	//m_RegionNodesActor->GetMapper()->SelectColorArray("colors");
	m_RegionNodesActor->GetMapper()->ScalarVisibilityOff();
	m_RegionNodesActor->GetProperty()->SetColor(0.95, 0.32, 0);
	m_RegionNodesActor->PickableOff();
	m_RegionNodesActor->Modified();
}

//! Cycles between values from 0.9 to 0
void iAVRVolume::filterRegionLinks()
{
	double step = 0.18;

	m_regionLinkDrawRadius -= step;
	if (m_regionLinkDrawRadius <= 0) m_regionLinkDrawRadius = 0.9;
}

double iAVRVolume::getJaccardFilterVal()
{
	return m_regionLinkDrawRadius;
}

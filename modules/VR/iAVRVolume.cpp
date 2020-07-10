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
#include <vtkVariantArray.h>
#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkMapper.h>
#include <vtkCubeSource.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkCellArray.h>
#include <vtkLine.h>

iAVRVolume::iAVRVolume(vtkRenderer* ren, vtkTable* objectTable, iACsvIO io) :m_objectTable(objectTable), m_io(io), iAVRCubicRepresentation{ren}
{
	m_volumeActor = vtkSmartPointer<vtkActor>::New();
	m_RegionLinksActor = vtkSmartPointer<vtkActor>::New();

	m_volumeVisible = false;
	m_regionLinksVisible = false;

	m_cylinderVis = new iA3DCylinderObjectVis(m_renderer, m_objectTable, m_io.getOutputMapping(), QColor(140, 140, 140, 255), std::map<size_t, std::vector<iAVec3f> >());
	m_volumeActor = m_cylinderVis->getActor();
}

void iAVRVolume::resetVolume()
{
	m_cylinderVis = new iA3DCylinderObjectVis(m_renderer, m_objectTable, m_io.getOutputMapping(), QColor(140, 140, 140, 255), std::map<size_t, std::vector<iAVec3f> >());
	m_volumeActor = m_cylinderVis->getActor();
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
	m_regionLinksVisible = true;
}

void iAVRVolume::hideRegionLinks()
{
	if (!m_regionLinksVisible)
	{
		return;
	}
	m_renderer->RemoveActor(m_RegionLinksActor);
	m_regionLinksVisible = false;
}

vtkSmartPointer<vtkActor> iAVRVolume::getVolumeActor()
{
	return m_volumeActor;
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

void iAVRVolume::createOctreeBoundingBox()
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
		DEBUG_LOG(QString("The Octree has no leaf node!"));
		return;
	}

	double regionSize[3] = { 0.0, 0.0, 0.0 };
	m_octree->calculateOctreeRegionSize(regionSize);
	calculateStartPoints();

	vtkSmartPointer<vtkCubeSource> cubeSource = vtkSmartPointer<vtkCubeSource>::New();
	cubeSource->SetXLength(regionSize[0]);
	cubeSource->SetYLength(regionSize[1]);
	cubeSource->SetZLength(regionSize[2]);
	cubeSource->Update();

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
	m_actor->GetProperty()->SetRepresentationToWireframe();
	m_actor->GetProperty()->SetRenderLinesAsTubes(true);
	m_actor->GetProperty()->SetLineWidth(6);
	m_actor->GetProperty()->SetColor(0.6, 0.8, 0.2);

}

void iAVRVolume::renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem)
{
	m_cylinderVis->renderSelection(sortedSelInds, classID, classColor, activeClassItem);
}

void iAVRVolume::createNewVolume(std::vector<size_t> fiberIDs)
{
	hide();

	vtkSmartPointer<vtkTable> reducedTable = vtkSmartPointer<vtkTable>::New();
	//reducedTable->DeepCopy(m_objectTable);
	//reducedTable->SetNumberOfRows(fiberIDs.size());

	DEBUG_LOG(QString("Number Of Columns = %1").arg(m_objectTable->GetNumberOfColumns()));

	for (int column = 0; column < m_objectTable->GetNumberOfColumns(); column++)
	{
		vtkSmartPointer<vtkFloatArray> col = vtkSmartPointer<vtkFloatArray>::New();
		col->SetName(m_objectTable->GetColumn(column)->GetName());

		for(int row = 0; row < fiberIDs.size(); row++)
		{
			col->InsertNextValue(0);
		}

		//reducedTable->AddColumn(m_objectTable->GetColumn(column));
		reducedTable->AddColumn(col);
	}

	DEBUG_LOG(QString("Number Of Rows = %1").arg(reducedTable->GetNumberOfRows()));

	for (int i = 0; i < fiberIDs.size(); i++)
	{
		DEBUG_LOG(QString("fiber = [%1]").arg(fiberIDs.at(i)));
		reducedTable->SetRow(i, m_objectTable->GetRow(fiberIDs.at(i) - 1));

		DEBUG_LOG(QString("fiber nr in new table = [%1]").arg(reducedTable->GetValue(i, 0).ToFloat()));
		DEBUG_LOG(QString("Value 1 in new table = [%1]").arg(reducedTable->GetValue(i, 1).ToFloat()));
	}

	m_cylinderVis = new iA3DCylinderObjectVis(m_renderer, reducedTable, m_io.getOutputMapping(), QColor(140, 140, 140, 255), std::map<size_t, std::vector<iAVec3f> >());
	m_volumeActor = m_cylinderVis->getActor();
	show();
}

//! Moves each octree region and its fiber from the octree center away
//! Should only be called if the mappers are set!
void iAVRVolume::moveRegions(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset)
{
	double maxLength = m_octree->getMaxDistanceOctCenterToRegionCenter();// m_octree->getMaxDistanceOctCenterToFiber();
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
	
	m_cylinderVis->getPolyData()->Modified();
	//m_octree->getOctree()->Modified();
}

//! Moves each octree region and its fiber from the octree center away
//! Should only be called if the mappers are set!
void iAVRVolume::moveFibersbyAllCoveredRegions(double offset)
{
	double maxLength = m_octree->getMaxDistanceOctCenterToRegionCenter();
	double centerPoint[3];
	double regionCenterPoint[3];
	m_octree->calculateOctreeCenterPos(centerPoint);
	iAVec3d centerPos = iAVec3d(centerPoint);

	std::vector<std::unordered_map<vtkIdType, double>*>* fiberCoverage = m_octree->getfibersInRegionMapping(&m_pointIDToCsvIndex);

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

		for (auto element : *fiberCoverage->at(region))
		{
			//DEBUG_LOG(QString("\n\nFiber <%1> is in Region").arg(element.first));

			auto findKeys = m_csvIndexToPointID.equal_range(element.first);
			for (auto it = findKeys.first; it != findKeys.second; ++it) {

				//DEBUG_LOG(QString("It has PolyPointID %1").arg(it->second));
				iAVec3d currentPoint = iAVec3d(m_cylinderVis->getPolyData()->GetPoint(it->second));

				m_octree->calculateOctreeRegionCenterPos(region, regionCenterPoint);
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
	m_cylinderVis->getPolyData()->Modified();
	//m_octree->getOctree()->Modified();
}

void iAVRVolume::moveBoundingBox(double offset)
{
	if (m_cubePolyData == nullptr)
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

void iAVRVolume::moveBoundingBoxRelative(double offset)
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

		iAVec3d move = normDirection * offset * (currentLength / maxLength);
		iAVec3d newPoint = currentPoint + move;

		glyph3D->GetPolyDataInput(0)->GetPoints()->SetPoint(i, newPoint.data());
	}
	m_cubePolyData->Modified();
}

void iAVRVolume::createRegionLinks()
{
	vtkSmartPointer<vtkPolyData> linesPolyData = vtkSmartPointer<vtkPolyData>::New();
	linesPolyData->SetPoints(m_cubePolyData->GetPoints());
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

	for (int i = 0; i < m_cubePolyData->GetNumberOfPoints(); i++)
	{
		for (int j = 0 + i; j < m_cubePolyData->GetNumberOfPoints(); j++)
		{
			vtkSmartPointer<vtkLine> l = vtkSmartPointer<vtkLine>::New();
			l->GetPointIds()->SetId(0, i);
			l->GetPointIds()->SetId(1, j);
			lines->InsertNextCell(l);
		}
	}
	linesPolyData->SetLines(lines);

	// Create a mapper and actor
	vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	lineMapper->SetInputData(linesPolyData);

	m_RegionLinksActor->SetMapper(lineMapper);
	m_RegionLinksActor->Modified();
	m_RegionLinksActor->GetProperty()->SetRenderLinesAsTubes(true);
	m_RegionLinksActor->GetProperty()->SetLineWidth(3);
	m_RegionLinksActor->GetProperty()->SetColor(0.980, 0.607, 0);
	//m_RegionLinksActor->GetProperty()->SetOpacity(0.7);
}

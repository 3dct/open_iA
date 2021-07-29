/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAVRMip.h"
#include <iAVec3.h>

#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataAlgorithm.h>
#include <vtkAppendPolyData.h>
#include <vtkCellData.h>
#include <vtkProperty.h>

iAVRMip::iAVRMip(vtkRenderer* renderer, std::vector<iAVROctree*>* octrees, iAVRColorLegend* colorLegend) :m_renderer(renderer), m_octrees(octrees), m_colorLegend(colorLegend)
{
	mipPlanes = std::vector<vtkPolyData*>();
}

//! Adds a colorLegend for the LUT information
void iAVRMip::addColorLegend(iAVRColorLegend* colorLegend)
{
	m_colorLegend = colorLegend;
}

//! Creates a plane for every possible MIP Projection (six planes)
//! The plane cells start at the lower left cell depending on the origin Point of the Plane
void iAVRMip::createMIPPanels(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues)
{
	int gridSize = pow(2, octreeLevel);
	double direction[6] = { 1,1,-1,-1,-1,1 }; //normals
	int viewPlane[6] = { 0,2,0,2,1,1 }; //x,y,z

	std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
	m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(planePoints);

	vtkSmartPointer<vtkAppendPolyData> appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();

	for (int i = 0; i < 6; i++)
	{
		vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
		plane->SetXResolution(gridSize);
		plane->SetYResolution(gridSize);
		plane->SetOrigin(planePoints->at(i).at(0).data());
		plane->SetPoint1(planePoints->at(i).at(1).data());
		plane->SetPoint2(planePoints->at(i).at(2).data());
		plane->Push(620 * direction[i]);
		plane->Update();

		vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
		colorData->SetName("colors");
		colorData->SetNumberOfComponents(4);

		std::vector<QColor>* col = calculateMIPColoring(viewPlane[i], octreeLevel, feature, calculatedValues);
		//for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
		for (int gridCell = 0; gridCell < gridSize * gridSize; gridCell++)
		{
			colorData->InsertNextTuple4(col->at(gridCell).red(), col->at(gridCell).green(), col->at(gridCell).blue(), col->at(gridCell).alpha());
		}

		plane->GetOutput()->GetCellData()->SetScalars(colorData);

		mipPlanes.push_back(plane->GetOutput());
		appendFilter->AddInputData(plane->GetOutput());
	}

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(appendFilter->GetOutputPort());

	mipPanel = vtkSmartPointer<vtkActor>::New();
	mipPanel->SetMapper(mapper);
	mipPanel->PickableOff();
	mipPanel->GetProperty()->SetOpacity(0.65);
	mipPanel->GetProperty()->EdgeVisibilityOn();
	mipPanel->GetProperty()->SetLineWidth(4);

	m_renderer->AddActor(mipPanel);
}

//! Creates a plane for the MIP Projection for the current viewed direction
//! The plane cells start at the lower left cell depending on the origin Point of the Plane
void iAVRMip::createSingleMIPPanel(int octreeLevel, int feature, int viewDir, double physicalScale, std::vector<std::vector<std::vector<double>>>* calculatedValues)
{
	hideMIPPanels();

	double planeOffset = physicalScale * 0.9;
	int gridSize = pow(2, octreeLevel);
	double direction[6] = { 1,1,-1,-1,-1,1 }; //normals
	int viewPlane[6] = { 0,2,0,2,1,1 }; //x,y,z

	std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
	m_octrees->at(octreeLevel)->createOctreeBoundingBoxPlanes(planePoints);

	vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
	plane->SetXResolution(gridSize);
	plane->SetYResolution(gridSize);
	plane->SetOrigin(planePoints->at(viewDir).at(0).data());
	plane->SetPoint1(planePoints->at(viewDir).at(1).data());
	plane->SetPoint2(planePoints->at(viewDir).at(2).data());
	plane->Push(planeOffset * direction[viewDir]);
	plane->Update();

	vtkSmartPointer<vtkUnsignedCharArray> colorData = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colorData->SetName("colors");
	colorData->SetNumberOfComponents(4);

	std::vector<QColor>* col = calculateMIPColoring(viewPlane[viewDir], octreeLevel, feature, calculatedValues);
	//for (int region = 0; region < m_fiberCoverage->at(octreeLevel).size(); region++)
	for (int gridCell = 0; gridCell < gridSize * gridSize; gridCell++)
	{
		colorData->InsertNextTuple4(col->at(gridCell).red(), col->at(gridCell).green(), col->at(gridCell).blue(), col->at(gridCell).alpha());
	}

	plane->GetOutput()->GetCellData()->SetScalars(colorData);

	mipPlanes.push_back(plane->GetOutput());

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(plane->GetOutput());

	mipPanel = vtkSmartPointer<vtkActor>::New();
	mipPanel->SetMapper(mapper);
	mipPanel->PickableOff();
	mipPanel->GetProperty()->SetOpacity(0.65);
	mipPanel->GetProperty()->EdgeVisibilityOn();
	mipPanel->GetProperty()->SetLineWidth(4);

	m_renderer->AddActor(mipPanel);
}

//! Hides the MIP panels from the user
void iAVRMip::hideMIPPanels()
{
	m_renderer->RemoveActor(mipPanel);
}

//! Calculates the maximum Intensity Projection for the chosen feature and direction (x = 0, y = 1, z = 2)
//! Saves the color for the maximum value found by shooting a parallel ray through a cube row.
std::vector<QColor>* iAVRMip::calculateMIPColoring(int direction, int level, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues)
{
	size_t gridSize = pow(2, level);
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>>* regionsInLine = m_octrees->at(level)->getRegionsInLineOfRay();
	std::vector<QColor>* mipColors = new std::vector<QColor>();
	mipColors->reserve(gridSize * gridSize);

	for (int y = 0; y < gridSize; y++)
	{
		for (int x = 0; x < gridSize; x++)
		{
			double val = 0;

			for (auto region : regionsInLine->at(direction).at(y).at(x))
			{
				double temp = calculatedValues->at(level).at(feature).at(region);
				if (val < temp) val = temp;
			}

			mipColors->push_back(m_colorLegend->getColor(val));
		}
	}

	return mipColors;
}
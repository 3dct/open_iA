// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRMip.h"

#include "iAVRColorLegend.h"
#include "iAVROctree.h"

#include <iAVec3.h>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCellData.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iAVRMip::iAVRMip(vtkRenderer* renderer, std::vector<iAVROctree*> const & octrees, iAVRColorLegend* colorLegend) :
	m_renderer(renderer), m_octrees(octrees), m_colorLegend(colorLegend), m_mipPlanes()
{
}

//! Adds a colorLegend for the LUT information
void iAVRMip::addColorLegend(iAVRColorLegend* colorLegend)
{
	m_colorLegend = colorLegend;
}

//! Creates a plane for every possible MIP Projection (six planes)
//! The plane cells start at the lower left cell depending on the origin Point of the Plane
void iAVRMip::createMIPPanels(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>> const & calculatedValues)
{
	int gridSize = pow(2, octreeLevel);
	double direction[6] = { 1,1,-1,-1,-1,1 }; //normals
	int viewPlane[6] = { 0,2,0,2,1,1 }; //x,y,z

	std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
	m_octrees.at(octreeLevel)->createOctreeBoundingBoxPlanes(planePoints);

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

		std::vector<QColor> col = calculateMIPColoring(viewPlane[i], octreeLevel, feature, calculatedValues);
		//for (int region = 0; region < m_objectCoverage->at(octreeLevel).size(); region++)
		for (int gridCell = 0; gridCell < gridSize * gridSize; gridCell++)
		{
			colorData->InsertNextTuple4(col.at(gridCell).red(), col.at(gridCell).green(), col.at(gridCell).blue(), col.at(gridCell).alpha());
		}

		plane->GetOutput()->GetCellData()->SetScalars(colorData);

		m_mipPlanes.push_back(plane->GetOutput());
		appendFilter->AddInputData(plane->GetOutput());
	}

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputConnection(appendFilter->GetOutputPort());

	m_mipPanel = vtkSmartPointer<vtkActor>::New();
	m_mipPanel->SetMapper(mapper);
	m_mipPanel->PickableOff();
	m_mipPanel->GetProperty()->SetOpacity(0.65);
	m_mipPanel->GetProperty()->EdgeVisibilityOn();
	m_mipPanel->GetProperty()->SetLineWidth(4);

	m_renderer->AddActor(m_mipPanel);
}

//! Creates a plane for the MIP Projection for the current viewed direction
//! The plane cells start at the lower left cell depending on the origin Point of the Plane
void iAVRMip::createSingleMIPPanel(int octreeLevel, int feature, int viewDir, double physicalScale, std::vector<std::vector<std::vector<double>>> const & calculatedValues)
{
	hideMIPPanels();

	double planeOffset = physicalScale * 0.9;
	int gridSize = pow(2, octreeLevel);
	double direction[6] = { 1,1,-1,-1,-1,1 }; //normals
	int viewPlane[6] = { 0,2,0,2,1,1 }; //x,y,z

	std::vector<std::vector<iAVec3d>>* planePoints = new std::vector<std::vector<iAVec3d>>();
	m_octrees.at(octreeLevel)->createOctreeBoundingBoxPlanes(planePoints);

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

	std::vector<QColor> col = calculateMIPColoring(viewPlane[viewDir], octreeLevel, feature, calculatedValues);
	//for (int region = 0; region < m_objectCoverage->at(octreeLevel).size(); region++)
	for (int gridCell = 0; gridCell < gridSize * gridSize; gridCell++)
	{
		colorData->InsertNextTuple4(col.at(gridCell).red(), col.at(gridCell).green(), col.at(gridCell).blue(), col.at(gridCell).alpha());
	}

	plane->GetOutput()->GetCellData()->SetScalars(colorData);

	m_mipPlanes.push_back(plane->GetOutput());

	vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	mapper->SetInputData(plane->GetOutput());

	m_mipPanel = vtkSmartPointer<vtkActor>::New();
	m_mipPanel->SetMapper(mapper);
	m_mipPanel->PickableOff();
	m_mipPanel->GetProperty()->SetOpacity(0.65);
	m_mipPanel->GetProperty()->EdgeVisibilityOn();
	m_mipPanel->GetProperty()->SetLineWidth(4);

	m_renderer->AddActor(m_mipPanel);
}

//! Hides the MIP panels from the user
void iAVRMip::hideMIPPanels()
{
	m_renderer->RemoveActor(m_mipPanel);
}

//! Calculates the maximum Intensity Projection for the chosen feature and direction (x = 0, y = 1, z = 2)
//! Saves the color for the maximum value found by shooting a parallel ray through a cube row.
std::vector<QColor> iAVRMip::calculateMIPColoring(int direction, int level, int feature, std::vector<std::vector<std::vector<double>>> const & calculatedValues)
{
	size_t gridSize = pow(2, level);
	std::vector<std::vector<std::vector<std::forward_list<vtkIdType>>>> const & regionsInLine = m_octrees.at(level)->getRegionsInLineOfRay();
	std::vector<QColor> mipColors;
	mipColors.reserve(gridSize * gridSize);

	for (size_t y = 0; y < gridSize; y++)
	{
		for (size_t x = 0; x < gridSize; x++)
		{
			double val = 0;

			for (auto region : regionsInLine.at(direction).at(y).at(x))
			{
				double temp = calculatedValues.at(level).at(feature).at(region);
				if (val < temp) val = temp;
			}

			mipColors.push_back(m_colorLegend->getColor(val));
		}
	}

	return mipColors;
}

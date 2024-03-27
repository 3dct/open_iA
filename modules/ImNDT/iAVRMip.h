// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QColor>

class vtkActor;
class vtkPolyData;
class vtkRenderer;

class iAVRColorLegend;
class iAVROctree;

//! Calculates and displays the Maximum Intensity Projection (MIP)
class iAVRMip
{
public:
	iAVRMip(vtkRenderer* renderer, std::vector<iAVROctree*> const & octrees, iAVRColorLegend* colorLegend);
	//! Adds a colorLegend for the LUT information
	void addColorLegend(iAVRColorLegend* colorLegend);
	//! Creates a plane for every possible MIP Projection (six planes)
	//! The plane cells start at the lower left cell depending on the origin Point of the Plane
	//void createMIPPanels(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>> const & calculatedValues);
	//! Creates a plane for the MIP Projection for the current viewed direction
	//! The plane cells start at the lower left cell depending on the origin Point of the Plane
	void createSingleMIPPanel(vtkIdType octreeLevel, vtkIdType feature, int viewDir, double physicalScale, std::vector<std::vector<std::vector<double>>> const & calculatedValues);
	//! Hides the MIP panels from the user
	void hideMIPPanels();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	std::vector<iAVROctree*> const & m_octrees;
	iAVRColorLegend* m_colorLegend;
	vtkSmartPointer<vtkActor> m_mipPanel;
	std::vector<vtkPolyData*> m_mipPlanes;

	//! Calculates the maximum Intensity Projection for the chosen feature and direction (x = 0, y = 1, z = 2)
	//! Saves the color for the maximum value found by shooting a parallel ray through a cube row.
	std::vector<QColor> calculateMIPColoring(int direction, vtkIdType level, vtkIdType feature, std::vector<std::vector<std::vector<double>>> const & calculatedValues);
};

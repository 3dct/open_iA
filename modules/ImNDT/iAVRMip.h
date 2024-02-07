// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <iAVRColorLegend.h>
#include <iAVROctree.h>

/*
* This class calculates and displays the Maximum Intensity Projection (MIP)
*/
class iAVRMip
{
public:
	iAVRMip(vtkRenderer* renderer, std::vector<iAVROctree*>* octrees, iAVRColorLegend* colorLegend);
	void addColorLegend(iAVRColorLegend* colorLegend);
	void createMIPPanels(int octreeLevel, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues);
	void createSingleMIPPanel(int octreeLevel, int feature, int viewDir, double physicalScale, std::vector<std::vector<std::vector<double>>>* calculatedValues);
	void hideMIPPanels();

private:
	vtkSmartPointer<vtkRenderer> m_renderer;
	std::vector<iAVROctree*>* m_octrees;
	iAVRColorLegend* m_colorLegend;
	vtkSmartPointer<vtkActor> mipPanel;
	std::vector<vtkPolyData*> mipPlanes;

	std::vector<QColor>* calculateMIPColoring(int direction, int level, int feature, std::vector<std::vector<std::vector<double>>>* calculatedValues);
};

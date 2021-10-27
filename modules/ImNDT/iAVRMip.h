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


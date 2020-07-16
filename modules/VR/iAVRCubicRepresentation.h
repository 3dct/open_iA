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
#pragma once

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkDataSet.h>
#include <vtkGlyph3D.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedCharArray.h>

#include "iAVROctree.h"

#include <QColor>
#include <unordered_map>

//! Base class for Objects which are represented through cubes (octree shape)
class iAVRCubicRepresentation
{
public:
	iAVRCubicRepresentation(vtkRenderer* ren);
	void setOctree(iAVROctree* octree);
	virtual void createCubeModel();
	void show();
	void hide();
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* fiberCoverage);
	vtkSmartPointer<vtkActor> getActor();
	vtkSmartPointer<vtkPolyData> getDataSet();
	vtkIdType getClosestCellID(double pos[3], double eventOrientation[3]);
	void setCubeColor(QColor col, int regionID);
	void applyHeatmapColoring(std::vector<QColor>* colorPerRegion);
	void highlightGlyphs(std::vector<vtkIdType>* regionIDs);
	void removeHighlightedGlyphs();
	void redrawHighlightedGlyphs();

	void applyLinearCubeOffset(double offset);
	void applyRelativeCubeOffset(double offset);
	void apply4RegionCubeOffset(double offset);

private:
	

protected:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkActor> m_activeActor;
	vtkSmartPointer<vtkPolyData> m_cubePolyData;
	vtkSmartPointer<vtkGlyph3D> glyph3D;
	vtkSmartPointer<vtkGlyph3D> activeGlyph3D;
	iAVROctree* m_octree;
	vtkSmartPointer<vtkDoubleArray> glyphScales;
	vtkSmartPointer<vtkUnsignedCharArray> glyphColor;
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>>* m_fiberCoverage;
	//Currently selected cubes
	std::vector<vtkIdType> activeRegions;
	QColor defaultColor;
	bool m_visible;
	bool m_highlightVisible;
	double defaultActorSize[3]; // Initial resize of all cube

	void calculateStartPoints();
	void drawPoint(std::vector<double*>* pos, QColor color);
};
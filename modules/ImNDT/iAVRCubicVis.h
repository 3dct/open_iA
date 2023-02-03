// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
#include "iAVRColorLegend.h"

#include <QColor>
#include <unordered_map>

//! Base class for Objects which are represented through cubes (octree shape)
class iAVRCubicVis
{
public:
	iAVRCubicVis(vtkRenderer* ren);
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
	void highlightGlyphs(std::vector<vtkIdType>* regionIDs, std::vector<QColor>* colorPerRegion = new std::vector<QColor>());
	void removeHighlightedGlyphs();
	void redrawHighlightedGlyphs();
	double* getDefaultActorSize();
	void applyRadialDisplacement(double offset);
	void applySPDisplacement(double offset);
	void applyOctantDisplacement(double offset);

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
	std::vector<QColor> activeColors;
	QColor defaultColor;
	bool m_visible;
	bool m_highlightVisible;
	double defaultActorSize[3]; // Initial resize of all cube

	void calculateStartPoints();
	void drawPoint(std::vector<double*>* pos, QColor color);
};

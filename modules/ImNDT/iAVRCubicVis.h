// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVROctree.h"

#include <vtkSmartPointer.h>

#include <QColor>

#include <unordered_map>

class vtkActor;
class vtkDoubleArray;
class vtkGlyph3D;
class vtkPolyData;
class vtkRenderer;
class vtkUnsignedCharArray;

//! Base class for Objects which are represented through cubes (octree shape)
class iAVRCubicVis
{
public:
	iAVRCubicVis(vtkRenderer* ren);
	void setOctree(iAVROctree* octree);
	//! Sets up the cubic representation of given points. The calculated points (from the octree) are displayed as glyphs (cubes) and saved in an actor.
	virtual void createCubeModel();
	//! Displays the cubes
	void show();
	//! Hides the cubes
	void hide();
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * fiberCoverage);
	vtkSmartPointer<vtkActor> getActor();
	vtkSmartPointer<vtkPolyData> getDataSet();
	vtkIdType getClosestCellID(double pos[3], double eventOrientation[3]);
	void setCubeColor(QColor col, int regionID);
	void applyHeatmapColoring(std::vector<QColor> const & colorPerRegion);
	void highlightGlyphs(std::vector<vtkIdType> const & regionIDs, std::vector<QColor> colorPerRegion = std::vector<QColor>());
	void removeHighlightedGlyphs();
	void redrawHighlightedGlyphs();
	double* getDefaultActorSize();
	void applyRadialDisplacement(double offset);
	void applySPDisplacement(double offset);
	void applyOctantDisplacement(double offset);

protected:
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkActor> m_activeActor;
	vtkSmartPointer<vtkPolyData> m_cubePolyData;
	vtkSmartPointer<vtkGlyph3D> m_glyph3D;
	vtkSmartPointer<vtkGlyph3D> m_activeGlyph3D;
	iAVROctree* m_octree;
	vtkSmartPointer<vtkDoubleArray> m_glyphScales;
	vtkSmartPointer<vtkUnsignedCharArray> m_glyphColor;
	//Stores for the [octree level] in an [octree region] a map of its fiberIDs with their coverage
	std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * m_fiberCoverage;
	//Currently selected cubes
	std::vector<vtkIdType> m_activeRegions;
	std::vector<QColor> m_activeColors;
	QColor m_defaultColor;
	bool m_visible;
	bool m_highlightVisible;
	double m_defaultActorSize[3]; // Initial resize of all cube

	void calculateStartPoints();
};

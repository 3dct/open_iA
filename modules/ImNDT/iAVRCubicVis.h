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
	//! Sets the fiber coverage data, which is a vector for every octree level and each region, in which every fiber is stored with its coverage in that particular region.
	void setFiberCoverageData(std::vector<std::vector<std::unordered_map<vtkIdType, double>*>> const * fiberCoverage);
	//! Returns the Actor for the glyphs
	vtkSmartPointer<vtkActor> getActor();
	//! Returns the vtkPolyData for the center points of the glyphs
	vtkSmartPointer<vtkPolyData> getDataSet();
	//! Returns the closest cell of the Cube which gets intersected by a ray
	vtkIdType getClosestCellID(double pos[3], double eventOrientation[3]);
	//! Sets the color (rgba) of one cube in the Miniature Model. The other colors stay the same.
	//! The region ID of the octree is used
	void setCubeColor(QColor col, int regionID);
	//! Colors the whole miniature model with the given vector of rgba values ( between 0.0 and 1.0)
	//! Resets the current color of all cubes with the new colors!
	void applyHeatmapColoring(std::vector<QColor> const & colorPerRegion);
	//! Creates colored border around the given Cubes. If no/empty color vector or too few colors are given the additional borders are black
	void highlightGlyphs(std::vector<vtkIdType> const & regionIDs, std::vector<QColor> colorPerRegion = std::vector<QColor>());
	//! Removes the colored border around selected Cubes.
	void removeHighlightedGlyphs();
	//! Redraw the borders of the last selection in black color
	void redrawHighlightedGlyphs();
	double* getDefaultActorSize();
	//! Applies a linear shift: All regions are displaced by the same factor, regardless of their
	//! distance from the center of the fiber model. This Method calculates the direction from
	//! the center to its single cubes and shifts the cubes linear from the center away
	//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
	void applyRadialDisplacement(double offset);
	//! The structure preserving displacement (SP) increases the distance to the center
	//! and the regions, but the relative distances between the regions remain the same.
	//! The shifts is scaled relative to the maximal length from the center to one cube and
	//! shifts all cubes from the center away
	//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
	void applySPDisplacement(double offset);
	//! The octant displacement shifts the regions to the nearest octant out of 8 possible octants.
	//! The center of the fiber model is the origin of the three - dimensional Euclidean coordinate
	//! system, which forms eight octants through the three axial planes X, Y, and Z.
	//! The original (vtkPoint) values are taken (not the actors visible getPosition() values)
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

	//! Iterates through all leaf regions of the octree and stores its center point in an vtkPolyData
	//! It also calculates the region size and adds the scalar array for it
	void calculateStartPoints();
};

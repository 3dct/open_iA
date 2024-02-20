// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRCubicVis.h"

class vtkLookupTable;
class vtkPointData;
class vtkPoints;
class iAColoredPolyObjectVis;
class iAPolyObjectVisActor;

class QStandardItem;

//! Class which represents the rendered volume
class iAVRObjectModel: public iAVRCubicVis
{
public:
	iAVRObjectModel(vtkRenderer* ren, iAColoredPolyObjectVis* polyObject);
	void resetVolume();
	void showVolume();
	void hideVolume();
	void createCubeModel() override;
	void showRegionLinks();
	void hideRegionLinks();
	vtkSmartPointer<vtkActor> getVolumeActor();
	double* getCubePos(int region);
	double getCubeSize(int region);
	//! Colors the cube nodes with the given region IDs with a given color
	//! Both vectors must have equal length
	void setNodeColor(std::vector<vtkIdType> const & regions, std::vector<QColor> const & color);
	void resetNodeColor();
	iAColoredPolyObjectVis* getPolyObject();
	void renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem);
	//! Moves all fibers from the octree center away.
	//! The fibers belong to the region in which they have their maximum coverage
	//! The flag relativeMovement decides if the offset is applied to the relative (radial) octree region postion
	//! or linear (SP)
	//! Should only be called if the mappers are set!
	void moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>> const & m_maxCoverage, double offset, bool relativeMovement);
	//! Moves all fibers from the octree center away.
	//! The fibers belong to every region in which they have a coverage
	//! Should only be called if the mappers are set!
	void moveFibersbyAllCoveredRegions(double offset, bool relativeMovement);
	void moveFibersbyOctant(std::vector<std::vector<std::vector<vtkIdType>>> const & m_maxCoverage, double offset);

	void createSimilarityNetwork(std::vector<std::vector<std::vector<double>>> const & similarityMetric, double maxFibersInRegions, double worldSize);

	//! Cycles between values from 0.95 to 0
	//! The sign defines if the values are increased/decreased
	void filterRegionLinks(int sign);
	double getJaccardFilterVal() const;

private:
	vtkSmartPointer<vtkActor> m_volumeActor;
	vtkSmartPointer<vtkActor> m_RegionLinksActor;
	vtkSmartPointer<vtkActor> m_RegionNodesActor;
	iAColoredPolyObjectVis* m_polyObject;
	vtkSmartPointer<vtkPoints> m_initialPoints;
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkDoubleArray> m_nodeGlyphScales;
	vtkSmartPointer<vtkUnsignedCharArray> m_linkGlyphColor;
	vtkSmartPointer<vtkUnsignedCharArray> m_nodeGlyphColor;
	vtkSmartPointer<vtkUnsignedCharArray> m_nodeGlyphResetColor;
	vtkSmartPointer<vtkGlyph3D> m_nodeGlyph3D;
	bool m_volumeVisible;
	bool m_regionLinksVisible;
	double m_regionLinkDrawRadius;

	void createRegionLinks(std::vector<std::vector<std::vector<double>>> const & similarityMetric, double worldSize);
	void createRegionNodes(double maxFibersInRegions, double worldSize);
	//! Calculates the LUT for the regionLinks (0) and the regionNodes (1)
	void calculateNodeLUT(double min, double max, int colorScheme);
};

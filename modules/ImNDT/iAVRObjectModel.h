// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRCubicVis.h"
#include "iACsvConfig.h"

#include <vtkTable.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkLookupTable.h>


class vtkPointData;
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
	void setNodeColor(std::vector<vtkIdType> regions, std::vector<QColor> color);
	void resetNodeColor();
	iAColoredPolyObjectVis* getPolyObject();
	void renderSelection(std::vector<size_t> const& sortedSelInds, int classID, QColor const& classColor, QStandardItem* activeClassItem);
	void moveFibersByMaxCoverage(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset, bool relativMovement);
	void moveFibersbyAllCoveredRegions(double offset, bool relativMovement);
	void moveFibersbyOctant(std::vector<std::vector<std::vector<vtkIdType>>>* m_maxCoverage, double offset);
	
	void createSimilarityNetwork(std::vector<std::vector<std::vector<double>>>* similarityMetric, double maxFibersInRegions, double worldSize);
	
	void filterRegionLinks(int sign);
	double getJaccardFilterVal();

private:
	vtkSmartPointer<vtkActor> m_volumeActor;
	vtkSmartPointer<vtkActor> m_RegionLinksActor;
	vtkSmartPointer<vtkActor> m_RegionNodesActor;
	iAColoredPolyObjectVis* m_polyObject;
	vtkSmartPointer<vtkPoints> m_initialPoints;
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkDoubleArray> nodeGlyphScales;
	vtkSmartPointer<vtkUnsignedCharArray> linkGlyphColor;
	vtkSmartPointer<vtkUnsignedCharArray> nodeGlyphColor;
	vtkSmartPointer<vtkUnsignedCharArray> nodeGlyphResetColor;
	vtkSmartPointer<vtkGlyph3D> nodeGlyph3D;
	bool m_volumeVisible;
	bool m_regionLinksVisible;
	double m_regionLinkDrawRadius;

	void createRegionLinks(std::vector<std::vector<std::vector<double>>>* similarityMetric, double worldSize);
	void createRegionNodes(double maxFibersInRegions, double worldSize);
	void calculateNodeLUT(double min, double max, int colorScheme);
};

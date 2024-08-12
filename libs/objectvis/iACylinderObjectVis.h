// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALineObjectVis.h"

class iAvtkTubeFilter;

//! Visualizes the objects given in a table as cylinders.
//!
//! Requires column mappings for start- and end point as well as for radius in the given object table data.
class iAobjectvis_API iACylinderObjectVis : public iALineObjectVis
{
public:
	static const int DefaultNumberOfCylinderSides = 12;
	//! Creates a cylinder object visualization.
	//! @param data the data of the objects
	//! @param color the general color of the visualization
	//! @param numberOfCylinderSides quality parameter for cylinder visualization (lower values lead to faster
	//!        rendering but also less quality; minimum value that makes sense is 3 (then you get a triangular tube)
	//! @param segmentSkip quality parameter for cylinder visualization (how many segments for curved fibers are
	//!        skipped; higher values lead to faster rendering but also less accurate representation of curved fibers
	//!        the default value of 1 means don't skip; larger values skip curved fiber segments
	iACylinderObjectVis(iAObjectsData const* data,
		QColor const & color, int numberOfCylinderSides = DefaultNumberOfCylinderSides, size_t segmentSkip = 1);
	virtual ~iACylinderObjectVis();
	void setDiameterFactor(double diameterFactor);
	void setContextDiameterFactor(double contextDiameterFactor);
	void setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive) override;
	QString visualizationStatistics() const override;
	vtkPolyData* finalPolyData() override;
	//vtkAlgorithmOutput* output() override;
	IndexType finalObjectStartPointIdx(IndexType objIdx) const override;
	IndexType finalObjectPointCount(IndexType objIdx) const override;
	std::vector<vtkSmartPointer<vtkPolyData>> extractSelectedObjects(QColor c) const override;

private:
	vtkSmartPointer<iAvtkTubeFilter> m_tubeFilter;
	float* m_contextFactors;
	IndexType m_objectCount;
	float m_contextDiameterFactor;
	//! maps the object ID to (first=) the first index in the points array that belongs to this object, and (second=) the number of final points
	std::vector<std::pair<IndexType, IndexType>> m_finalObjectPointMap;
};

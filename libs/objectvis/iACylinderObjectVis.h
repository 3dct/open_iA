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

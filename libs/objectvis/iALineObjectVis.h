// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAColoredPolyObjectVis.h"

#include <iAVec3.h>

#include <vtkSmartPointer.h>

class vtkPoints;

//! Visualizes the objects given in a table as lines.
//!
//! Requires column mappings for start- and end point in the given object table data.
class iAobjectvis_API iALineObjectVis : public iAColoredPolyObjectVis
{
public:
	// TODO: unify curved fiber data between here and updateValues!
	iALineObjectVis(iAObjectsData const* data, QColor const & color, size_t segmentSkip);
	void updateValues( std::vector<std::vector<double> > const & values, int straightOrCurved);
	vtkPolyData* polyData() override;
	vtkPolyData* finalPolyData() override;
	QString visualizationStatistics() const override;
	std::vector<vtkSmartPointer<vtkPolyData>> extractSelectedObjects(QColor c) const override;
	IndexType objectStartPointIdx(IndexType objIdx) const override;
	IndexType objectPointCount(IndexType objIdx) const override;

protected:
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkPoints> m_points;

private:
	//! maps the object ID to (first=) the first index in the points array that belongs to this object, and (second=) the number of points
	std::vector<std::pair<IndexType, IndexType>> m_objectPointMap;
	IndexType m_totalNumOfSegments;
};

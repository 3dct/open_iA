// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA3DColoredPolyObjectVis.h"

#include <iAVec3.h>

#include <vtkSmartPointer.h>

class vtkPoints;

class iAobjectvis_API iA3DLineObjectVis : public iA3DColoredPolyObjectVis
{
public:
	// TODO: unify curved fiber data between here and updateValues!
	iA3DLineObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
		QColor const & color, std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, size_t segmentSkip );
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
	std::map<size_t, std::vector<iAVec3f>> m_curvedFiberData;

private:
	//! maps the object ID to (first=) the first index in the points array that belongs to this object, and (second=) the number of points
	std::vector<std::pair<IndexType, IndexType>> m_objectPointMap;
	IndexType m_totalNumOfSegments;
};

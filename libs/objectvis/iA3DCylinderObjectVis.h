// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA3DLineObjectVis.h"

class iAvtkTubeFilter;

class iAobjectvis_API iA3DCylinderObjectVis : public iA3DLineObjectVis
{
public:
	static const int DefaultNumberOfCylinderSides = 12;
	iA3DCylinderObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
		QColor const & color, std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData,
		int numberOfCylinderSides = DefaultNumberOfCylinderSides, size_t segmentSkip = 1);
	virtual ~iA3DCylinderObjectVis();
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

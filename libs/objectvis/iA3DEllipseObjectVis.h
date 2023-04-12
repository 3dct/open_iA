// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA3DColoredPolyObjectVis.h"

#include <vtkSmartPointer.h>

class vtkPolyData;

//! Visualizes the objects given in a table as ellipses.
//!
//! Requires column mappings for center point coordinates and a dimension in each
//! 3D direction in the given object table data
class iAobjectvis_API iA3DEllipseObjectVis : public iA3DColoredPolyObjectVis
{
public:
	static const int DefaultPhiRes = 10;
	static const int DefaultThetaRes = 10;
	iA3DEllipseObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
		QColor const & color, int phiRes = DefaultPhiRes, int thetaRes = DefaultThetaRes);
	double const * bounds() override;
	vtkPolyData* polyData() override;
	vtkPolyData* finalPolyData() override;
	QString visualizationStatistics() const override;
	std::vector<vtkSmartPointer<vtkPolyData>> extractSelectedObjects(QColor c) const override;
	IndexType objectStartPointIdx(IndexType objIdx) const override;
	IndexType objectPointCount(IndexType objIdx) const override;

private:
	vtkSmartPointer<vtkPolyData> m_fullPoly;
	IndexType m_pointsPerEllipse;
};

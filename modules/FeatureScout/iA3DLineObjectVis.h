/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iA3DColoredPolyObjectVis.h"

#include <iAvec3.h>

#include <vtkSmartPointer.h>

class vtkPoints;

class FeatureScout_API iA3DLineObjectVis: public iA3DColoredPolyObjectVis
{
public:
	// TODO: unify curved fiber data between here and updateValues!
	iA3DLineObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
		QColor const & color, std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, size_t segmentSkip );
	void updateValues( std::vector<std::vector<double> > const & values, int straightOrCurved);
	vtkPolyData* getPolyData() override;
	QString visualizationStatistics() const override;
protected:
	IndexType objectStartPointIdx(IndexType objIdx) const override;
	IndexType objectPointCount(IndexType objIdx) const override;
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkPoints> m_points;
private:
	//! maps the object ID to (first=) the first index in the points array that belongs to this object, and (second=) the number of points
	std::vector<std::pair<IndexType, IndexType>> m_objectPointMap;
	IndexType m_totalNumOfSegments;
};


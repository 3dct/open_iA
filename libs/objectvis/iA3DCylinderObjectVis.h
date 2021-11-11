/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
	std::vector<vtkSmartPointer<vtkPolyData>> extractSelectedObjects(QColor c) const override;

private:
	vtkSmartPointer<iAvtkTubeFilter> m_tubeFilter;
	float* m_contextFactors;
	IndexType m_objectCount;
	float m_contextDiameterFactor;
	bool m_lines;
};


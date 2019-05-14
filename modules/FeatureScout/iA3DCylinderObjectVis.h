/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iA3DLineObjectVis.h"

class iAvtkTubeFilter;

class FeatureScout_API iA3DCylinderObjectVis: public iA3DLineObjectVis
{
private:
	double m_diameterFactor = 1;
	vtkSmartPointer<iAvtkTubeFilter> m_tubeFilter;
	float* m_contextFactors;
	size_t m_objectCount;
	float m_contextDiameterFactor;
	std::map<size_t, std::vector<iAVec3f> > m_curvedFiberData;
public:
	static const int DefaultNumberOfCylinderSides = 12;
	iA3DCylinderObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
		QColor const & color, std::map<size_t, std::vector<iAVec3f> > curvedFiberData,
		int numberOfCylinderSides = DefaultNumberOfCylinderSides);
	void setDiameterFactor(double diameterFactor);
	void setContextDiameterFactor(double contextDiameterFactor);
	void setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive) override;
};


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
#include "iA3DNoVis.h"

iA3DNoVis::iA3DNoVis():iA3DObjectVis(nullptr, nullptr, QSharedPointer<QMap<uint,uint>>())
{
	std::fill(m_dummyBounds, m_dummyBounds + 3, 0);
	std::fill(m_dummyBounds +3, m_dummyBounds + 6, 1);
}

void iA3DNoVis::renderSelection(std::vector<size_t> const & /*sortedSelInds*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iA3DNoVis::renderSingle(int /*labelID*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iA3DNoVis::multiClassRendering(QList<QColor> const & /*classColors*/, QStandardItem* /*rootItem*/, double /*alpha*/)
{}

void iA3DNoVis::renderOrientationDistribution(vtkImageData* /*oi*/)
{}

void iA3DNoVis::renderLengthDistribution(vtkColorTransferFunction* /*cTFun*/, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{}

double const * iA3DNoVis::bounds()
{
	return m_dummyBounds;
}
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
#include "iA3DObjectFactory.h"

#include "iA3DLabelledVolumeVis.h"
#include "iA3DLineObjectVis.h"
#include "iA3DCylinderObjectVis.h"
#include "iA3DNoVis.h"
#include "iA3DEllipseObjectVis.h"
#include "iACsvConfig.h"

QSharedPointer<iA3DObjectVis> create3DObjectVis(int visualization, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip,
	vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, double const* bounds)
{
	switch (visualization)
	{
	default:
	case iACsvConfig::UseVolume:
		return QSharedPointer<iA3DLabelledVolumeVis>::create(ctf, otf, table, columnMapping, bounds);
	case iACsvConfig::Lines:
		return QSharedPointer<iA3DLineObjectVis>::create(table, columnMapping, color, curvedFiberInfo, segmentSkip);
	case iACsvConfig::Cylinders:
		return QSharedPointer<iA3DCylinderObjectVis>::create(
			table, columnMapping, color, curvedFiberInfo, numberOfCylinderSides, segmentSkip);
	case iACsvConfig::Ellipses:
		return QSharedPointer<iA3DEllipseObjectVis>::create(table, columnMapping, color);
	case iACsvConfig::NoVis:
		return QSharedPointer<iA3DNoVis>::create();
	}
}
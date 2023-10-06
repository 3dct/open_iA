// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectVisFactory.h"

#include "iALabeledVolumeVis.h"
#include "iALineObjectVis.h"
#include "iACylinderObjectVis.h"
#include "iANoObjectVis.h"
#include "iAEllipsoidObjectVis.h"
#include "iACsvConfig.h"

#include <iALog.h>

QSharedPointer<iAObjectVis> create3DObjectVis(iAObjectVisType visType, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip,
	vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, double const* bounds)
{
	auto data = std::make_shared<iAObjectsData>(table, columnMapping);
	switch (visType)
	{
	default:
	case iAObjectVisType::UseVolume:
		if (!ctf || !otf || !bounds)
		{
			LOG(lvlWarn, "Labelled Volume visualization requested, but transfer functions or bounds missing. Disabling 3D visualization!");
			return QSharedPointer<iANoObjectVis>::create();
		}
		return QSharedPointer<iALabeledVolumeVis>::create(ctf, otf, data, bounds);
	case iAObjectVisType::Line:
		return QSharedPointer<iALineObjectVis>::create(data, color, curvedFiberInfo, segmentSkip);
	case iAObjectVisType::Cylinder:
		return QSharedPointer<iACylinderObjectVis>::create(
			data, color, curvedFiberInfo, numberOfCylinderSides, segmentSkip);
	case iAObjectVisType::Ellipsoid:
		return QSharedPointer<iAEllipsoidObjectVis>::create(data, color);
	case iAObjectVisType::None:
		return QSharedPointer<iANoObjectVis>::create();
	}
}

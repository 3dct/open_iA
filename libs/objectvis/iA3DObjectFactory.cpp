// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DObjectFactory.h"

#include "iA3DLabelledVolumeVis.h"
#include "iA3DLineObjectVis.h"
#include "iA3DCylinderObjectVis.h"
#include "iA3DNoVis.h"
#include "iA3DEllipseObjectVis.h"
#include "iACsvConfig.h"

#include <iALog.h>

QSharedPointer<iA3DObjectVis> create3DObjectVis(iAObjectVisType visType, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip,
	vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, double const* bounds)
{
	auto data = std::make_shared<iA3DObjectsData>(table, columnMapping);
	switch (visType)
	{
	default:
	case iAObjectVisType::UseVolume:
		if (!ctf || !otf || !bounds)
		{
			LOG(lvlWarn, "Labelled Volume visualization requested, but transfer functions or bounds missing. Disabling 3D visualization!");
			return QSharedPointer<iA3DNoVis>::create();
		}
		return QSharedPointer<iA3DLabelledVolumeVis>::create(ctf, otf, data, bounds);
	case iAObjectVisType::Lines:
		return QSharedPointer<iA3DLineObjectVis>::create(data, color, curvedFiberInfo, segmentSkip);
	case iAObjectVisType::Cylinders:
		return QSharedPointer<iA3DCylinderObjectVis>::create(
			data, color, curvedFiberInfo, numberOfCylinderSides, segmentSkip);
	case iAObjectVisType::Ellipses:
		return QSharedPointer<iA3DEllipseObjectVis>::create(data, color);
	case iAObjectVisType::NoVis:
		return QSharedPointer<iA3DNoVis>::create();
	}
}

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

QSharedPointer<iA3DObjectVis> create3DObjectVis(int visualization, vtkTable* table,
	QSharedPointer<QMap<uint, uint>> columnMapping, QColor const& color,
	std::map<size_t, std::vector<iAVec3f>>& curvedFiberInfo, int numberOfCylinderSides, size_t segmentSkip,
	vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, double const* bounds)
{
	switch (visualization)
	{
	default:
	case iACsvConfig::UseVolume:
		if (!ctf || !otf || !bounds)
		{
			LOG(lvlWarn, "Labelled Volume visualization requested, but transfer functions or bounds missing. Disabling 3D visualization!");
			return QSharedPointer<iA3DNoVis>::create();
		}
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

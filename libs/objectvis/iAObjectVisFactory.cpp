// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectVisFactory.h"

#include "iACylinderObjectVis.h"
#include "iAEllipsoidObjectVis.h"
#include "iALabeledVolumeVis.h"
#include "iALineObjectVis.h"
#include "iANoObjectVis.h"
#include "iAObjectsData.h"

#include <iALog.h>

std::shared_ptr<iAObjectVis> create3DObjectVis(iAObjectsData const * data, QColor const& color,
	int numberOfCylinderSides, size_t segmentSkip,
	vtkColorTransferFunction* ctf, vtkPiecewiseFunction* otf, double const* bounds)
{
	switch (data->m_visType)
	{
	default:
	case iAObjectVisType::UseVolume:
		if (!ctf || !otf || !bounds)
		{
			LOG(lvlWarn, "Labelled Volume visualization requested, but transfer functions or bounds missing. Disabling 3D visualization!");
			return std::make_shared<iANoObjectVis>();
		}
		return std::make_shared<iALabeledVolumeVis>(ctf, otf, data, bounds);
	case iAObjectVisType::Line:
		return std::make_shared<iALineObjectVis>(data, color, segmentSkip);
	case iAObjectVisType::Cylinder:
		return std::make_shared<iACylinderObjectVis>(data, color, numberOfCylinderSides, segmentSkip);
	case iAObjectVisType::Ellipsoid:
		return std::make_shared<iAEllipsoidObjectVis>(data, color);
	case iAObjectVisType::None:
		return std::make_shared<iANoObjectVis>();
	}
}

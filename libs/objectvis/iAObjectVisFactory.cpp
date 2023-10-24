// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectVisFactory.h"

#include "iACylinderObjectVis.h"
#include "iAEllipsoidObjectVis.h"
#include "iALineObjectVis.h"
#include "iAObjectsData.h"

#include <iALog.h>

std::shared_ptr<iAColoredPolyObjectVis> createObjectVis(iAObjectsData const * data,
	QColor const& color, int numberOfCylinderSides, size_t segmentSkip)
{
	switch (data->m_visType)
	{
	case iAObjectVisType::Line:      return std::make_shared<iALineObjectVis>(data, color, segmentSkip);
	case iAObjectVisType::Cylinder:	 return std::make_shared<iACylinderObjectVis>(data, color, numberOfCylinderSides, segmentSkip);
	case iAObjectVisType::Ellipsoid: return std::make_shared<iAEllipsoidObjectVis>(data, color);
	default:
	case iAObjectVisType::None:      return std::shared_ptr<iAColoredPolyObjectVis>();
	}
}

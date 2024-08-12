// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectVisFactory.h"

#include "iACylinderObjectVis.h"
#include "iAEllipsoidObjectVis.h"
#include "iALineObjectVis.h"
#include "iANoObjectVis.h"
#include "iAObjectsData.h"
#include "iAObjectsViewer.h"

#include <iADefaultSettings.h>

#include <iALog.h>

std::shared_ptr<iAObjectVis> createObjectVis(iAObjectsData const * data, QColor const& color)
{
	// TODO: take config values from data, only take default values if no values there
	auto const & attr = iAObjectsRenderer::defaultAttributes();
	auto numOfCylinderSides = getValue(attr, iAObjectsRenderer::NumOfCylinderSides).toInt();
	auto segmentSkip = getValue(attr, iAObjectsRenderer::SegmentSkip).toULongLong();
	switch (data->m_visType)
	{
	case iAObjectVisType::Line:      return std::make_shared<iALineObjectVis>(data, color, segmentSkip);
	case iAObjectVisType::Cylinder:	 return std::make_shared<iACylinderObjectVis>(data, color, numOfCylinderSides, segmentSkip);
	case iAObjectVisType::Ellipsoid: return std::make_shared<iAEllipsoidObjectVis>(data, color);
	default:
	case iAObjectVisType::None:      return std::shared_ptr<iANoObjectVis>();
	}
}

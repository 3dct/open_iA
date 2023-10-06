// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include <QString>

//! Types of objects currently supported by the object visualization framework.
enum iAObjectType
{
	InvalidObjectType = -1,
	Fibers,
	Voids,
	Other
};

//! Map a value from iAObjectType to a string representation for display to the user.
iAobjectvis_API QString MapObjectTypeToString(int objectType);
//! Map a string value to the corresponding iAObjectType.
iAobjectvis_API iAObjectType MapStringToObjectType(QString const& objectTypeName);

enum class iAObjectVisType
{
	UseVolume,
	Line,
	Cylinder,
	Ellipsoid,
	None,
	Count //must be last element
}; //!< what visualization to use for the objects. Should match the entries of VisualizationTypeName iACsvConfig.cpp

//! Map an object visualization type to string
iAobjectvis_API QString MapVisType2Str(iAObjectVisType visType);
iAobjectvis_API iAObjectVisType MapStr2VisType(QString const & name);

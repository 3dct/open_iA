// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAobjectvis_export.h"

#include <QString>

enum iAObjectType
{
	InvalidObjectType = -1,
	Fibers,
	Voids,
	Other
};

iAobjectvis_API QString MapObjectTypeToString(int objectType);
iAobjectvis_API iAObjectType MapStringToObjectType(QString const& objectTypeName);

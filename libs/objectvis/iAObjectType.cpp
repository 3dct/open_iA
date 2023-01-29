// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectType.h"

QString MapObjectTypeToString(int objectType)
{
	return (objectType == iAObjectType::Fibers) ? "Fibers" :
		(objectType == iAObjectType::Voids) ? "Voids" :
		(objectType == iAObjectType::Other) ? "Other" : "Invalid";
}

iAObjectType MapStringToObjectType(QString const & objectTypeName)
{
	if (objectTypeName == "Voids")
		return iAObjectType::Voids;
	else if (objectTypeName == "Fibers")
		return iAObjectType::Fibers;
	else if (objectTypeName == "Other")
		return iAObjectType::Other;
	else
		return iAObjectType::InvalidObjectType;
}

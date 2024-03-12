// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectType.h"

namespace
{
	const char * VisualizationTypeName[static_cast<int>(iAObjectVisType::Count)] =
	{
		"Labelled Volume",
		"Lines",
		"Cylinders",
		"Ellipsoids",
		"No Visualization"
	};
}

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


QString MapVisType2Str(iAObjectVisType visType)
{
	return VisualizationTypeName[static_cast<int>(visType)];
}

iAObjectVisType MapStr2VisType(QString const & name)
{
	for (int i = 0; i < static_cast<int>(iAObjectVisType::Count); ++i)
	{
		if (name == VisualizationTypeName[i])
		{
			return static_cast<iAObjectVisType>(i);
		}
	}
	return iAObjectVisType::UseVolume;
}

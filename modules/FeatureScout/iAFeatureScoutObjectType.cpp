/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAFeatureScoutObjectType.h"

#include <QColor>

QString MapObjectTypeToString(int objectType)
{
	return (objectType == iAFeatureScoutObjectType::Fibers) ? "Fibers" :
		(objectType == iAFeatureScoutObjectType::Voids) ? "Voids" :
		(objectType == iAFeatureScoutObjectType::Other) ? "Other" : "Invalid";
}

iAFeatureScoutObjectType MapStringToObjectType(QString const & objectTypeName)
{
	if (objectTypeName == "Voids")
		return iAFeatureScoutObjectType::Voids;
	else if (objectTypeName == "Fibers")
		return iAFeatureScoutObjectType::Fibers;
	else if (objectTypeName == "Other")
		return iAFeatureScoutObjectType::Other;
	else
		return iAFeatureScoutObjectType::InvalidObjectType;
}

QColor getClassColor(int cid)
{
	// automatically select a predefined color
	// (from the list of colors defined in the list of SVG
	// color keyword names provided by the World Wide Web Consortium).
	//http://www.w3.org/TR/SVG/types.html#ColorKeywords
	if (cid > 7) { cid = 1; }
	switch (cid)
	{
		default:
		case 1: return QColor("cornflowerblue");
		case 2: return QColor("darkorange");
		case 3: return QColor("chartreuse");
		case 4: return QColor("yellow");
		case 5: return QColor("mediumvioletred");
		case 6: return QColor("blue");
		case 7: return QColor("green");
	}
}

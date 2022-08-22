/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#pragma once

#include "iAbase_export.h"

enum class iAValueType
{
	Invalid = -1,
	Continuous,
	Discrete,
	Categorical,
	String,
	Text,
	Boolean,
	FilterName,
	FilterParameters,
	Folder,
	FileNameOpen,
	FileNamesOpen,
	FileNameSave,
	Color,
	// better way?
	Vector2,    // vector of 2 continuous values
	Vector3,    // vector of 3 continuous values
	Vector2i,   // vector of 2 discrete values
	Vector3i    // vector of 3 discrete values
};

class QString;

// TODO: use something like magic enum (https://github.com/Neargye/magic_enum) instead to convert between enum/string?
iAbase_API QString ValueType2Str(iAValueType type);
iAValueType Str2ValueType(QString const & str);

class QVariant;

//! convert the QVariant of the given iAValueType to a string (describing its content, for human-readable output)
iAbase_API QString variantValueToString(iAValueType valueType, QVariant value);

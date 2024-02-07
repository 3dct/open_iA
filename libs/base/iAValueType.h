// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

//! Value types that can be used for parameters and attributes
//! @see iAAttributeDescriptor
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
	Color,            //!< color value; use variantToColor/colorToVariant functions to get/set!
	// better way?
	Vector2,          //!< vector of 2 continuous values (see iAValueTypeVectorHelpers for functions to convert to/from QVariant!
	Vector3,          //!< vector of 3 continuous values (see iAValueTypeVectorHelpers for functions to convert to/from QVariant!
	Vector2i,         //!< vector of 2 discrete values (see iAValueTypeVectorHelpers for functions to convert to/from QVariant!
	Vector3i          //!< vector of 3 discrete values (see iAValueTypeVectorHelpers for functions to convert to/from QVariant!
};

class QString;

// TODO: use something like magic enum (https://github.com/Neargye/magic_enum) instead to convert between enum/string?
iAbase_API QString ValueType2Str(iAValueType type);
iAValueType Str2ValueType(QString const & str);

class QVariant;

//! convert the QVariant of the given iAValueType to a string (describing its content, for human-readable output)
iAbase_API QString variantValueToString(iAValueType valueType, QVariant value);

class QColor;

//! Convert QVariant with value type iAValueType::Color (internally stored as string) to a QColor
iAbase_API QColor variantToColor(QVariant const& v);
//! Convert a given QColor to its internally used QVariant representation (a string)
iAbase_API QVariant colorToVariant(QColor const& c);

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

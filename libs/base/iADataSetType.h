// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QFlags>

//! Dataset type contained in files
enum class iADataSetType
{
	None = 0,
	Volume = 0x1,
	Mesh = 0x2,
	Graph = 0x4,
	Collection = 0x8,
	SingleDataSets = Volume | Mesh | Graph,
	All = Volume | Mesh | Graph | Collection
};
Q_DECLARE_FLAGS(iADataSetTypes, iADataSetType)
Q_DECLARE_OPERATORS_FOR_FLAGS(iADataSetTypes)

// Copyright (c) open_iA contributors
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
	GeometricObject = 0x8,    // for single simple geometric objects (e.g. sphere, cone, line); currently no file format supported (possibly .geo? but that can have multiple geometric objects)
	Collection = 0x10,        // a collection of other datasets
	Objects = 0x20,           // a list of objects (e.g. fibers, pores), as stored in a FiberScout .csv file
	SingleDataSets = Volume | Mesh | Graph,
	All = Volume | Mesh | Graph | Collection
};
Q_DECLARE_FLAGS(iADataSetTypes, iADataSetType)
Q_DECLARE_OPERATORS_FOR_FLAGS(iADataSetTypes)

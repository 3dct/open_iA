// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include <QString>

//! Parameters for loading/saving file stacks.
class iAio_API iAFileStackParams
{
public:
	static QString const FileNameBase;
	static QString const Extension;
	static QString const NumDigits;
	static QString const MinimumIndex;
	static QString const MaximumIndex;
};

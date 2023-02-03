// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

class QString;

//! Abstract interface for mapping each index from a given range [0..size()-1] to names.
class iAbase_API iANameMapper
{
public:
	virtual ~iANameMapper() {}
	virtual QString name(int idx) const = 0;
	virtual int GetIdx(QString const & name, bool & ok) const = 0;
	virtual int size() const =0;
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVisModuleItem.h"

#include "iAVisModule.h"

iAVisModuleItem::iAVisModuleItem( iAVisModule * module, QString name, int id /*= 0*/ )
{
	this->module = module;
	this->name = name;
	this->id = id;
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVisModuleItem.h"

#include "iAVisModule.h"

iAVisModuleItem::iAVisModuleItem( iAVisModule * module, QString name, int id )
{
	this->module = module;
	this->name = name;
	this->id = id;
}

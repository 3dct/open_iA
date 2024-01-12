// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilterRegistry.h"

#include "iALog.h"
#include "iAFilter.h"

namespace
{	// if the data structures here would be members of iAFilterRegistry, we would run into the "Static Initialization Order Fiasco"!
	std::vector<iAFilterCreateFuncPtr> & filters()
	{
		static std::vector<iAFilterCreateFuncPtr> filterList;
		return filterList;
	}
}

bool iAFilterRegistry::add(iAFilterCreateFuncPtr filterCreateFunc)
{
	filters().push_back(filterCreateFunc);
	return true;
}

std::vector<iAFilterCreateFuncPtr> const & iAFilterRegistry::filterFactories()
{
	return filters();
}

std::shared_ptr<iAFilter> iAFilterRegistry::filter(QString const & name)
{
	int id = filterID(name);
	return id == -1 ? std::shared_ptr<iAFilter>() : filters()[id]();
}

int iAFilterRegistry::filterID(QString const & name)
{
	int cur = 0;
	for (auto filterCreateFunc : filters())
	{
		auto filter = filterCreateFunc();
		if (filter->name() == name)
		{
			return cur;
		}
		++cur;
	}
	LOG(lvlError, QString("Filter '%1' not found!").arg(name));
	return -1;
}

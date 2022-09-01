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
#include "iAFilterRegistry.h"

#include "iALog.h"
#include "iAFilter.h"

namespace
{
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

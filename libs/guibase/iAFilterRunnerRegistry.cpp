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
#include "iAFilterRunnerRegistry.h"

#include "iAFilterRunnerGUI.h"

#include <QMap>

namespace
{
	QMap<int, iAFilterRunnerGUICreateFuncPtr> & runners()
	{
		static QMap<int, iAFilterRunnerGUICreateFuncPtr> s_runner;
		return s_runner;
	}
}

void iAFilterRunnerRegistry::add(int filterID, iAFilterRunnerGUICreateFuncPtr runnerCreateFunc)
{
	runners().insert(filterID, runnerCreateFunc);
}

std::shared_ptr<iAFilterRunnerGUI> iAFilterRunnerRegistry::filterRunner(int filterID)
{
	if (runners().contains(filterID))
	{
		return runners()[filterID]();
	}
	else
	{
		return std::make_shared<iAFilterRunnerGUI>();
	}
}

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
#include "iAToolRegistry.h"

#include "iALog.h"
#include "iATool.h"

#include <cassert>

#include <QMap>

namespace
{// if the data structures here would be members of iAToolRegistry, we would run into the "Static Initialization Order Fiasco"!
	QMap<QString, iAToolCreateFuncPtr>& toolTypes()
	{
		static QMap<QString, iAToolCreateFuncPtr> s_toolTypes;
		return s_toolTypes;
	}
}

void iAToolRegistry::addTool(QString const& toolIdentifier, iAToolCreateFuncPtr toolCreateFunc)
{
	if (toolTypes().contains(toolIdentifier))
	{
		LOG(lvlWarn, QString("Trying to add already registered project type %1 again!").arg(toolIdentifier));
	}
	toolTypes().insert(toolIdentifier, toolCreateFunc);
}

QList<QString> const iAToolRegistry::toolKeys()
{
	return toolTypes().keys();
}

std::shared_ptr<iATool> iAToolRegistry::createTool(QString const & toolIdentifier)
{
	assert(toolTypes().contains(toolIdentifier));
	return toolTypes()[toolIdentifier]();
}



iATool::iATool():
	m_mdiChild(nullptr),
	m_mainWindow(nullptr)
{}

iATool::~iATool()
{}

void iATool::loadState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(projectFile);
	Q_UNUSED(fileName);
}
void iATool::saveState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(projectFile);
	Q_UNUSED(fileName);
}

void iATool::setChild(iAMdiChild* child)
{
	m_mdiChild = child;
}

void iATool::setMainWindow(iAMainWindow* mainWindow)
{
	m_mainWindow = mainWindow;
}

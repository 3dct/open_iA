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
#include "iAProjectRegistry.h"

#include "iALog.h"
#include "iAProjectBase.h"

#include <cassert>

#include <QMap>

namespace
{
	QMap<QString, iAProjectCreateFuncPtr>& projectTypes()
	{
		static QMap<QString, iAProjectCreateFuncPtr> s_projectTypes;
		return s_projectTypes;
	}
}

void iAProjectRegistry::addProject(QString const& projectIdentifier, iAProjectCreateFuncPtr projectCreateFunc)
{
	if (projectTypes().contains(projectIdentifier))
	{
		LOG(lvlWarn, QString("Trying to add already registered project type %1 again!").arg(projectIdentifier));
	}
	projectTypes().insert(projectIdentifier, projectCreateFunc);
}

QList<QString> const iAProjectRegistry::projectKeys()
{
	return projectTypes().keys();
}

std::shared_ptr<iAProjectBase> iAProjectRegistry::createProject(QString const & projectIdentifier)
{
	assert(projectTypes().contains(projectIdentifier));
	return projectTypes()[projectIdentifier]();
}



iAProjectBase::iAProjectBase():
	m_mdiChild(nullptr),
	m_mainWindow(nullptr)
{}

iAProjectBase::~iAProjectBase()
{}


void iAProjectBase::setChild(iAMdiChild* child)
{
	m_mdiChild = child;
}

void iAProjectBase::setMainWindow(iAMainWindow* mainWindow)
{
	m_mainWindow = mainWindow;
}

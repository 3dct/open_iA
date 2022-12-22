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
#include "iAFiAKErTool.h"

#include "iAFiAKErController.h"
#include "iAFiAKErModuleInterface.h"

#include "iALog.h"
#include "iAMainWindow.h"
#include "iAModuleDispatcher.h"
#include "iASettings.h"    // for mapFromQSettings

#include <QObject>

iAFiAKErTool::iAFiAKErTool(iAMainWindow* mainWnd, iAMdiChild* child):
	iATool(mainWnd, child),
	m_controller(new iAFiAKErController(mainWnd, child))
{
}

std::shared_ptr<iATool> iAFiAKErTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAFiAKErTool>(mainWnd, child);
}

iAFiAKErTool::~iAFiAKErTool()
{
	delete m_controller;
}

iAFiAKErController* iAFiAKErTool::controller()
{
	return m_controller;
}

void iAFiAKErTool::loadState(QSettings& projectFile, QString const& fileName)
{
	/*
	// Remove UseMdiChild setting altogether, always open iAMdiChild?
	if (projectFile.contains("UseMdiChild") && projectFile.value("UseMdiChild").toBool() == false)
	{

		QMessageBox::warning(nullptr, "FiAKEr", "Old project file detected (%1). "
			"Due to an implementation change, this file cannot be loaded directly; "
			"please open it in a text editor and remove the ")
		return;
	}
	*/
	if (!m_child)
	{
		LOG(lvlError, QString("Invalid FIAKER project file '%1': FIAKER requires a child window, "
			"but UseMdiChild was apparently not specified in this project, as no child window available! "
			"Please report this error, along with the project file, to the open_iA developers!").arg(fileName));
		return;
	}
	iAFiAKErModuleInterface* fiaker = m_mainWindow->moduleDispatcher().module<iAFiAKErModuleInterface>();
	fiaker->setupToolBar();
	//m_mainWindow->setPath(m_lastPath);
	auto projectSettings = mapFromQSettings(projectFile);
	QObject::connect(m_controller, &iAFiAKErController::setupFinished, [this, projectSettings, fileName]
		{
			m_controller->loadAdditionalData(projectSettings, fileName);
		});
	m_controller->loadProject(projectFile, fileName);
}

void iAFiAKErTool::saveState(QSettings& projectFile, QString const& fileName)
{
	m_controller->saveProject(projectFile, fileName);
}

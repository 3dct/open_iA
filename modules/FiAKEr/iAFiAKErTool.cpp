// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	auto fiaker = m_mainWindow->moduleDispatcher().module<iAFiAKErModuleInterface>();
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

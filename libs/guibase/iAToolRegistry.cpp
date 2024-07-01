// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

std::shared_ptr<iATool> iAToolRegistry::createTool(QString const & toolIdentifier, iAMainWindow* mainWnd, iAMdiChild* child)
{
	assert(toolTypes().contains(toolIdentifier));
	return toolTypes()[toolIdentifier](mainWnd, child);
}


#include <QSettings>

namespace
{
	QString ProjectToolActive("Active");
}

iATool::iATool(iAMainWindow* mainWnd, iAMdiChild* child):
	m_child(child), m_mainWindow(mainWnd)
{}

iATool::~iATool()
{}

void iATool::loadState(QSettings & projectFile, QString const& fileName)
{
	Q_UNUSED(projectFile);
	Q_UNUSED(fileName);
}

void iATool::saveState(QSettings& projectFile, QString const& fileName)
{
	Q_UNUSED(fileName);
	projectFile.setValue(ProjectToolActive, "true");  // just to create the tool's section!
}

/*
bool  iATool::waitForRendering() const
{
	return true;
}
*/

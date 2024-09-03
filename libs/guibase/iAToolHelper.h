// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iALog.h>

#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAQMenuHelper.h"

#include <QMessageBox>

//! helper function to add a tool to the current mdi child (if it exists)
template <typename T>
T* addToolToActiveMdiChild(QString const & name, iAMainWindow* mainWnd, bool unique = true)
{
	auto child = mainWnd->activeMdiChild();
	if (!child)
	{
		LOG(lvlWarn, QString("Cannot start tool %1: Open data window required, but none found!").arg(name));
		return nullptr;
	}
	if (unique && getTool<T>(child))
	{
		LOG(lvlWarn, QString("Tool %1: Already attached to the current window, only one instance allowed!").arg(name));
		return nullptr;
	}
	try
	{
		auto t = std::make_shared<T>(mainWnd, child);
		child->addTool(name, t);
		return t.get();
	}
	catch (std::exception const& e)
	{
		QMessageBox::warning(mainWnd, name, e.what());
		LOG(lvlError, QString("While starting tool %1, an error occurred: %2").arg(name).arg(e.what()));
	}
	return nullptr;
}

template <class ToolType>
void addToolAction(iAMainWindow* mainWnd, QMenu* menu, bool unique = true, QString menuTitle = QString(), QString toolTitle = QString())
{
	QAction* action = new QAction(menuTitle.isEmpty() ? ToolType::Name: menuTitle, mainWnd);
	QObject::connect(action, &QAction::triggered, mainWnd, [mainWnd, toolTitle, unique]()
	{
		addToolToActiveMdiChild<ToolType>(toolTitle.isEmpty() ? ToolType::Name: toolTitle, mainWnd, unique);
	});
	mainWnd->makeActionChildDependent(action);
	addToMenuSorted(menu, action);
}

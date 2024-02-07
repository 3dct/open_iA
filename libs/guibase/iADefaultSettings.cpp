// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefaultSettings.h"

#include "iAMainWindow.h"
#include "iAParameterDlg.h"
#include "iASettings.h"

#include "iAQMenuHelper.h"

#include <QAction>
#include <QMenu>
#include <QSettings>

namespace
{	// if the data structures here would be members of iASettingsManager, we would run into the "Static Initialization Order Fiasco"!
	iASettingsManager::iASettingsMap& defaultSettingsMap()
	{
		static iASettingsManager::iASettingsMap mymap;
		return mymap;
	}

	void loadSettings(QString const& fullName)
	{
		QSettings settings;
		auto attrPtr = defaultSettingsMap()[fullName];
		settings.beginGroup("Settings/" + configStorageName(fullName));
		auto valueMap = mapFromQSettings(settings);
		setDefaultValues(*attrPtr, valueMap);
		settings.endGroup();
	}

	void createMenuEntry(QString const& fullName)
	{
		QStringList catName = fullName.split("/", Qt::SkipEmptyParts);
		QString name = catName[catName.size() - 1];
		auto editArgsAction = new QAction(name);
		auto mainWnd = iAMainWindow::get();
		QObject::connect(editArgsAction, &QAction::triggered, mainWnd,
			[fullName, mainWnd]
			{
				iASettingsManager::editDefaultSettings(mainWnd, fullName);
			});
		QMenu* settingsMenu = mainWnd->editMenu();
		QStringList categories = catName.sliced(0, catName.size() - 1);
		for (auto cat : categories)
		{
			assert(!cat.isEmpty());   // if (cat.isEmpty()) LOG(lvlDebug, QString("Filter %1: Invalid category %2 - empty section!").arg(filter->name()).arg(filter->fullCategory()));
			settingsMenu = getOrAddSubMenu(settingsMenu, cat);
		}
		addToMenuSorted(settingsMenu, editArgsAction);
	}
}

iASettingsManager::iASettingsMap const& iASettingsManager::getMap()
{
	return defaultSettingsMap();
}

bool iASettingsManager::add(QString const& name, iAAttributes* attributes)
{
	defaultSettingsMap().insert(name, attributes);
	if (m_initialized)
	{
		loadSettings(name);
		createMenuEntry(name);
	}
	return true;
}

void iASettingsManager::init()
{
	for (auto fullName : getMap().keys())
	{
		loadSettings(fullName);
		createMenuEntry(fullName);
	}
	m_initialized = true;
}

void iASettingsManager::editDefaultSettings(QWidget* parent, QString const& fullName)
{
	auto attrPtr = getMap()[fullName];
	iAParameterDlg dlg(parent, fullName, *attrPtr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto newDefaultValues = dlg.parameterValues();
	setDefaultValues(*attrPtr, newDefaultValues);
}

void iASettingsManager::store()
{
	QSettings settings;
	settings.remove("Settings"); // clean up previous settings; removes outdated settings and prevents case changes in a settings key to prevent successful loading
	for (auto key : defaultSettingsMap().keys())
	{
		auto attr = defaultSettingsMap()[key];
		storeSettings("Settings/" + configStorageName(key), extractValues(*attr));
	}
}

bool iASettingsManager::m_initialized = false;

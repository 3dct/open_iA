// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefaultSettings.h"

#include "iAMainWindow.h"
#include "iAParameterDlg.h"
#include "iASettings.h"

#include "iAQMenuHelper.h"

#include <QAction>
#include <QMenu>
#include <QSettings>

iASettingsManager::iASettingsMap& defaultSettingsInternal()
{
	static iASettingsManager::iASettingsMap mymap;
	return mymap;
}

iASettingsManager::iASettingsMap const& iASettingsManager::getMap()
{
	return defaultSettingsInternal();
}

void iASettingsManager::add(QString const& name, iAAttributes* attributes)
{
	defaultSettingsInternal().insert(name, attributes);
}

void iASettingsManager::init()
{
	QSettings settings;
	for (auto fullName : getMap().keys())
	{
		auto attrPtr = getMap()[fullName];
		settings.beginGroup("Settings/" + configStorageName(fullName));
		auto valueMap = mapFromQSettings(settings);
		setDefaultValues(*attrPtr, valueMap);

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
		settings.endGroup();
	}
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
	for (auto key : defaultSettingsInternal().keys())
	{
		auto attr = defaultSettingsInternal()[key];
		storeSettings("Settings/" + configStorageName(key), extractValues(*attr));
	}
}

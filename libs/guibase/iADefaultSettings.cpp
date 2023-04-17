// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefaultSettings.h"

#include "iAMainWindow.h"
#include "iAParameterDlg.h"
#include "iASettings.h"

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
	for (auto name : getMap().keys())
	{
		auto attrPtr = getMap()[name];
		settings.beginGroup("Defaults/" + name);
		auto valueMap = mapFromQSettings(settings);
		setDefaultValues(*attrPtr, valueMap);
		auto editArgsAction = new QAction(name);
		auto mainWnd = iAMainWindow::get();
		QObject::connect(editArgsAction, &QAction::triggered, mainWnd,
			[attrPtr, mainWnd, name]
			{
				iAParameterDlg dlg(mainWnd, "Default settings: " + name, *attrPtr);
				if (dlg.exec() != QDialog::Accepted)
				{
					return;
				}
				auto newDefaultValues = dlg.parameterValues();
				setDefaultValues(*attrPtr, newDefaultValues);
			});
		mainWnd->editMenu()->addAction(editArgsAction);
	}
}

void iASettingsManager::store()
{
	QSettings settings;
	for (auto key : defaultSettingsInternal().keys())
	{
		auto attr = defaultSettingsInternal()[key];
		storeSettings("Defaults/" + key, extractValues(*attr));
	}
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefaultSettings.h"

#include "iAMainWindow.h"
#include "iAParameterDlg.h"
#include "iASettings.h"

#include <QAction>
#include <QMenu>
#include <QSettings>

DefaultSettingsMap& defaultSettingsInternal()
{
	static DefaultSettingsMap mymap;
	return mymap;
}

DefaultSettingsMap const& defaultSettings()
{
	return defaultSettingsInternal();
}

void registerDefaultSettings(QString const& name, iAAttributes* attributes)
{
	defaultSettingsInternal().insert(name, attributes);
}

void initDefaultSettings()
{
	QSettings settings;
	for (auto name : defaultSettings().keys())
	{
		auto attr = defaultSettings()[name];
		settings.beginGroup("Defaults/" + name);
		auto valueMap = mapFromQSettings(settings);
		setDefaultValues(*attr, valueMap);
		auto editArgsAction = new QAction(name);
		auto attrPtr = defaultSettings()[name];
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

void storeDefaultSettings()
{
	QSettings settings;
	for (auto key : defaultSettingsInternal().keys())
	{
		auto attr = defaultSettingsInternal()[key];
		storeSettings("Defaults/" + key, extractValues(*attr));
	}
}

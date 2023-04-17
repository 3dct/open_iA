// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADefaultSettings.h"

#include "iAMainWindow.h"
#include "iAParameterDlg.h"
#include "iASettings.h"

#include <QAction>
#include <QMenu>
#include <QRegularExpression>
#include <QSettings>

namespace
{
	QString configStorageName(QString const& in)
	{
		QString out(in);
		return out.remove(QRegularExpression("[^a-zA-Z\\d]"));
	}
}

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
		settings.beginGroup("Settings/" + configStorageName(name));
		auto valueMap = mapFromQSettings(settings);
		setDefaultValues(*attrPtr, valueMap);
		auto editArgsAction = new QAction(name);
		auto mainWnd = iAMainWindow::get();
		QObject::connect(editArgsAction, &QAction::triggered, mainWnd,
			[attrPtr, mainWnd, name]
			{
				iAParameterDlg dlg(mainWnd, name, *attrPtr);
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
		storeSettings("Settings/" + configStorageName(key), extractValues(*attr));
	}
}

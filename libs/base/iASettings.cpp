// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASettings.h"

#include <QSettings>

QVariantMap mapFromQSettings(QSettings const & settings)
{
	QVariantMap result;
	for (QString key : settings.allKeys())
	{
		result[key] = settings.value(key);
	}
	return result;
}

void storeSettings(QString const& group, QVariantMap const& values)
{
	QSettings settings;
	settings.beginGroup(group);
	for (QString key : values.keys())
	{
		settings.setValue(key, values[key]);
	}
}

QVariantMap loadSettings(QString const& group, QVariantMap const& defaultValues)
{
	QSettings settings;
	settings.beginGroup(group);
	auto result = mapFromQSettings(settings);
	for (auto key : defaultValues.keys())
	{
		if (!result.contains(key))
		{
			result[key] = defaultValues[key];
		}
	}
	return result;
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QDataStream>    // required, otherwise "no operator>>" errors
#include <QMetaType>      // for qRegisterMetaTypeStreamOperators
#include <QVector>
#endif

void initializeSettingTypes()
{
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)    // stream operators automatically registered with Qt >= 6
	qRegisterMetaTypeStreamOperators<QVector<int>>("QVector<int>");
	qRegisterMetaTypeStreamOperators<QVector<double>>("QVector<double>");
#endif
}

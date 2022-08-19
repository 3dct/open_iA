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

QVariantMap loadSettings(QString const& group)
{
	QSettings settings;
	settings.beginGroup(group);
	return mapFromQSettings(settings);
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

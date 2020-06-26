/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "open_iA_Core_export.h"

#include "iASettings.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QSettings>

// To be able to put non-QObject derived class in settingsWidgetMap
class iAQCheckBoxVector : public QObject, public QVector<QCheckBox*> { };
class iAQLineEditVector : public QObject, public QVector<QLineEdit*> { };

using iAWidgetMap = QMap<QString, QObject*>;

open_iA_Core_API void loadSettings(iASettings const& settings, iAWidgetMap const& settingsWidgetMap);
open_iA_Core_API void saveSettings(QSettings& settings, iAWidgetMap const& settingsWidgetMap);
open_iA_Core_API void saveSettings(iASettings& settings, iAWidgetMap const& settingsWidgetMap);
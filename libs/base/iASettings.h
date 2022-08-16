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
#pragma once

#include "iAbase_export.h"

// TODO: replace with QHash?
#include <QMap>

class QSettings;
class QVariant;

typedef QMap<QString, QVariant> iASettings;

//! Retrieve a map of all values in (the current group of) a given QSettings object.
//! @param settings the QSettings object to load all settings from
//! @return a map containing key->value pairs for all settings currently in
//!     (current group of) the given QSettings object
iAbase_API iASettings mapFromQSettings(QSettings const& settings);

//! Load settings in the given group from the platform-specific storage
iAbase_API iASettings loadSettings(QString const& group);
//! Save the given setting values to the platform-specific storage
iAbase_API void storeSettings(QString const & group, iASettings const& values);
//! Initialize Qt meta types / serialization operators required for
//! storing and loading specific setting values
iAbase_API void initializeSettingTypes();

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

// TODO: replace with QHash?
#include <QMap>
#include <QVariant>    // for QVariantMap (at least under Qt 5.15.2)

class QSettings;

iAbase_API QString configStorageName(QString const& in);

//! Retrieve a map of all values in (the current group of) a given QSettings object.
//! @param settings the QSettings object to load all settings from
//! @return a map containing key->value pairs for all settings currently in
//!     (current group of) the given QSettings object
iAbase_API QVariantMap mapFromQSettings(QSettings const& settings);

//! Load settings in the given group from the platform-specific storage
//! @param group name of the group of settings to load from the platform-specific storage (see QSettings)
//! @param defaultValues values for keys not present in the platform-specific storage will be initialized from this parameter
iAbase_API QVariantMap loadSettings(QString const& group, QVariantMap const & defaultValues);
//! Save the given setting values to the platform-specific storage
iAbase_API void storeSettings(QString const & group, QVariantMap const& values);

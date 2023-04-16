// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <iAAttributes.h>

using DefaultSettingsMap = QMap<QString, iAAttributes*>;

//! Register a new list of default settings
iAguibase_API void registerDefaultSettings(QString const& name, iAAttributes* attributes);

//! Retrieve a map of all registered default settings
iAguibase_API DefaultSettingsMap const& defaultSettings();

//! Initialize default settings (load stored values, and create menu entry for modifying them)
iAguibase_API void initDefaultSettings();

//! Store default settings
iAguibase_API void storeDefaultSettings();
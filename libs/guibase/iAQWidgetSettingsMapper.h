// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QSettings>

//! Helper class to be able to put non-QObject class QVector<QCheckBox> in a iAQWidgetMap
class iAQCheckBoxVector : public QObject, public QVector<QCheckBox*> { };
//! Helper class to be able to put non-QObject class QVector<QRadioButton> in a iAQWidgetMap
class iAQRadioButtonVector : public QObject, public QVector<QRadioButton*> { };
//! Helper class to be able to put non-QObject class QVector<QLineEdit> in a iAQWidgetMap
class iAQLineEditVector : public QObject, public QVector<QLineEdit*> { };

using iAQWidgetMap = QMap<QString, QObject*>;

//! Takes values from given settings object and applies them to the respective widget
//! mapped to in the given settings-widget-map.
//! @param settings a hash map containing one key-value pair per setting stored in it
//! @param settingsWidgetMap maps from a settings key to the widget which represents this setting in the GUI
iAguibase_API void loadSettings(QVariantMap const& settings, iAQWidgetMap const& settingsWidgetMap);

//! Looks at the current value of the GUI elements given in the settings-widget-map,
//! and stores it under the respective keys in the settings object.
//! @param settings a QSettings object for storing the values from the GUI.
//! @param settingsWidgetMap maps from a settings key to the widget which represents this setting in the GUI.
iAguibase_API void saveSettings(QSettings& settings, iAQWidgetMap const& settingsWidgetMap);

//! Looks at the current value of the GUI elements given in the settings-widget-map,
//! and stores it under the respective keys in the settings object.
//! @param settings a hash map for storing the values from the GUI.
//! @param settingsWidgetMap maps from a settings key to the widget which represents this setting in the GUI.
iAguibase_API void saveSettings(QVariantMap& settings, iAQWidgetMap const& settingsWidgetMap);

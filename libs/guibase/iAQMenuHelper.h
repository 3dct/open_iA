// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

class QAction;
class QMenu;
class QString;

//! Search in the given menu for a menu with the given title; if it doesn't exist, add it (alphabetically sorted).
iAguibase_API QMenu* getOrAddSubMenu(QMenu* parentMenu, QString const& title, bool addSeparator = false);

//! Add a given action to a menu, such that the (previously sorted) menu stays alphabetically sorted.
iAguibase_API void addToMenuSorted(QMenu* menu, QAction* action);

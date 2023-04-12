// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <QList>

#include <memory>

class iAMainWindow;
class iAMdiChild;
class iATool;

class QString;

using iAToolCreateFuncPtr = std::shared_ptr<iATool>(*)(iAMainWindow* mainWnd, iAMdiChild* child);

//! Registry for descendants of iATool.
//! Used to identify available tools when reading a project
class iAguibase_API iAToolRegistry
{
public:
	//! Adds a given tool type to the registry.
	static void addTool(QString const & toolIdentifier, iAToolCreateFuncPtr toolCreateFunc);
	static QList<QString> const toolKeys();
	static std::shared_ptr<iATool> createTool(QString const& toolIdentifier, iAMainWindow* mainWnd, iAMdiChild* child);

private:
	iAToolRegistry() =delete;	//!< iAToolRegistry is meant to be used statically only, thus prevent creation of objects
};

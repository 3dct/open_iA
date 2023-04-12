// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <memory>

class iAFilterRunnerGUI;
using iAFilterRunnerGUICreateFuncPtr = std::shared_ptr<iAFilterRunnerGUI>(*)();

//! Registry for descendants of iAFilter, providing some processing of datasets.
//!
//! All filters in the registry are made available via the "Filters" menu,
//! and also on the command line.
class iAguibase_API iAFilterRunnerRegistry
{
public:
	//! Retrieve the callback for a given factory (if the given factory does not
	//! have a callback, nullptr is returned).
	static std::shared_ptr<iAFilterRunnerGUI> filterRunner(int filterID);

	static void add(int filterID, iAFilterRunnerGUICreateFuncPtr runnerCreateFunc);

private:
	iAFilterRunnerRegistry() =delete;  //!< iAFilterRunnerRegistry is meant to be used statically only, thus prevent creation of objects
};

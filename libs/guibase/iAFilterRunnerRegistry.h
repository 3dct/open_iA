// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <memory>

class iAFilterRunnerGUI;
using iAFilterRunnerGUICreateFuncPtr = std::shared_ptr<iAFilterRunnerGUI>(*)();

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

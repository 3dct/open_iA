// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAutoRegistration.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"

// Convenience macro for creating the declaration of an iAFilter, including auto-registration
#define IAFILTER_DEFAULT_CLASS(FilterName) \
class FilterName : public iAFilter, private iAAutoRegistration<iAFilter, FilterName, iAFilterRegistry> \
{ \
public: \
	FilterName(); \
	static std::shared_ptr<FilterName> create(); \
private: \
	void performWork(QVariantMap const & parameters) override; \
};

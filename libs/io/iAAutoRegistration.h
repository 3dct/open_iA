// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

//! Provides auto-registration capabilities to a given registry, just by deriving from iAAutoRegistration.
//!
//! Requires parameters for Base class, the type of the Class itself and the Registry type.
//! Uses the curiously recurring template pattern to automatically create a factory function for the given
//! Class with no parameters and returning a std::shared_ptr<Base>; the given Registry class is required
//! to have an add function that takes such a factory function as parameter.
//!
//! DUPLICATION to base/iAAutoRegistration!
template <class Base, class Class, class Registry>
class iAAutoRegistration
{
	static bool s_bRegistered;
	static std::shared_ptr<Base> create()
	{
		return std::make_shared<Class>();
	}
public:
	iAAutoRegistration()
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
		s_bRegistered;    // required for self registration - otherwise it will not be done.
#pragma GCC diagnostic pop
	}
};

template <class Base, class Class, class Registry>
bool iAAutoRegistration<Base, Class, Registry>::s_bRegistered = Registry::add(create);

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

//! Automatically register classes with a registry by deriving from this class.
//! As template parameters pass the classes Base class, the Class itself, and the Registry class.
//! Automatically creates a factory function with no parameters returning a std::shared_ptr<Base>
//! with an instance of Class; requires that the Registry has an `add` function that expects
//! such a factory function as parameter.
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

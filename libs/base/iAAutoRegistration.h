#pragma once

#include <memory>

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

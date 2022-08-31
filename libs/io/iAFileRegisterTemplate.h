#pragma once
#include "iAFileTypeRegistry.h"


//!Template to self register IO Type in the factory
//!This done by defining an static variable s_bRegistered and init it.

template <typename T>
class iAFileRegisterTemplate
{
protected:
	iAFileRegisterTemplate()
	{
		s_bRegistered; //Helper for self register otherwise it will note be done.
	}
    static bool s_bRegistered;
};

template <typename T>
bool iAFileRegisterTemplate<T>::s_bRegistered = iAFileTypeRegistry::addFileType<T>();
// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkCommand.h>

//! Throws an exception when observing an error in itk objects.
class iAExceptionThrowingErrorObserver : public vtkCommand
{
public:
	iAExceptionThrowingErrorObserver() {}
	static iAExceptionThrowingErrorObserver *New()
	{
		return new iAExceptionThrowingErrorObserver;
	}
	void Execute(vtkObject *vtkNotUsed(caller),
		unsigned long /*event*/,
		void *calldata) override
	{
		//assert(event == vtkCommand::ErrorEvent);
		throw std::runtime_error(static_cast<char *>(calldata));
	}
};

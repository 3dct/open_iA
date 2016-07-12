#pragma once

#include <vtkCommand.h>

#include <cassert>

class iAExceptionThrowingErrorObserver : public vtkCommand
{
public:
	iAExceptionThrowingErrorObserver() {}
	static iAExceptionThrowingErrorObserver *New()
	{
		return new iAExceptionThrowingErrorObserver;
	}
	virtual void Execute(vtkObject *vtkNotUsed(caller),
		unsigned long event,
		void *calldata)
	{
		assert(event == vtkCommand::ErrorEvent);
		throw std::exception(static_cast<char *>(calldata));
	}
};

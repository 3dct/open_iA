#pragma once

#include "iAConsole.h"

#include <vtkOutputWindow.h>
#include <vtkObjectFactory.h>

class iARedirectVtkOutput : public vtkOutputWindow
{
public:
	vtkTypeMacro(iARedirectVtkOutput, vtkOutputWindow);
	void PrintSelf(ostream& os, vtkIndent indent);
	static iARedirectVtkOutput * New();
	virtual void DisplayText(const char*);
private:
	iARedirectVtkOutput();
	iARedirectVtkOutput(const iARedirectVtkOutput &) = delete;
	void operator=(const iARedirectVtkOutput &) = delete;
};

vtkStandardNewMacro(iARedirectVtkOutput);

iARedirectVtkOutput::iARedirectVtkOutput() {}

void iARedirectVtkOutput::DisplayText(const char* someText)
{
	DEBUG_LOG(someText);
}

//----------------------------------------------------------------------------
void iARedirectVtkOutput::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}

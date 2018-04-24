/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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

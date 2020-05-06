/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAVRInteractorStyle.h"

#include <vtkObjectFactory.h>
#include <iAConsole.h>

vtkStandardNewMacro(iAVRInteractorStyle);

iAVRInteractorStyle::iAVRInteractorStyle():vtkOpenVRInteractorStyle()
{
	
}

void iAVRInteractorStyle::OnButton3D(vtkEventData* edata)
{
	vtkOpenVRInteractorStyle::OnButton3D(edata);
	
	vtkEventDataDevice3D *event = edata->GetAsEventDataDevice3D();

	// right trigger press/release
	if (event->GetAction() == vtkEventDataAction::Press)
	{
		ShowBillboard("TEXT CAN BE READ!!");
	}
	if (event->GetAction() == vtkEventDataAction::Release)
	{
		HideBillboard();
	}

}



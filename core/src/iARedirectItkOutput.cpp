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
#include "iARedirectItkOutput.h"

#include "iAConsole.h"

iARedirectItkOutput::Pointer iARedirectItkOutput::m_instance;

iARedirectItkOutput::Pointer iARedirectItkOutput::New()
{
	if (!m_instance)
	{
		iARedirectItkOutput::m_instance = new iARedirectItkOutput;
		iARedirectItkOutput::m_instance->UnRegister();
	}
	return m_instance;
}

void iARedirectItkOutput::DisplayDebugText(const char *t)
{
	DEBUG_LOG(QString("ITK %1").arg(t));
}

void iARedirectItkOutput::DisplayErrorText(const char *t)
{
	DEBUG_LOG(QString("ITK %1").arg(t));
}

void iARedirectItkOutput::DisplayGenericOutputText(const char *t)
{
	DEBUG_LOG(QString("ITK %1").arg(t));
}

void iARedirectItkOutput::DisplayText(const char * t)
{
	DEBUG_LOG(QString("ITK %1").arg(t));
}

void iARedirectItkOutput::DisplayWarningText(const char *t)
{
	DEBUG_LOG(QString("ITK %1").arg(t));
}

void iARedirectItkOutput::SetPromptUser(bool _arg)
{}

bool iARedirectItkOutput::GetPromptUser() const
{
	return false;
}

void iARedirectItkOutput::PromptUserOn()
{}

void iARedirectItkOutput::PromptUserOff()
{}

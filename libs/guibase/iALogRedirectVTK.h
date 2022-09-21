/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iALog.h>

#include <vtkOutputWindow.h>
#include <vtkObjectFactory.h>

//! Redirects all log output from VTK to the LOG macro (i.e. iALog)
class iALogRedirectVTK : public vtkOutputWindow
{
public:
	vtkTypeMacro(iALogRedirectVTK, vtkOutputWindow);
	static iALogRedirectVTK* New();
	void PrintSelf(ostream& os, vtkIndent indent) override
	{
		this->Superclass::PrintSelf(os, indent);
	}
	void DisplayText(const char* someText) override
	{
		if (!m_enabled)
		{
			return;
		}
		iALogLevel lvl = lvlWarn;
		switch (GetCurrentMessageType())
		{
		case MESSAGE_TYPE_TEXT           : lvl = lvlInfo;  break;
		case MESSAGE_TYPE_ERROR          : lvl = lvlError; break;
		default:
	#if __cplusplus >= 201703L
			[[fallthrough]];
	#endif
			// fall through
		case MESSAGE_TYPE_WARNING        :
	#if __cplusplus >= 201703L
			[[fallthrough]];
	#endif
			// fall through
		case MESSAGE_TYPE_GENERIC_WARNING: lvl = lvlWarn;  break;
		case MESSAGE_TYPE_DEBUG          : lvl = lvlDebug; break;
		}
		LOG(lvl, someText);
	}
	void setEnabled(bool enabled)
	{
		m_enabled = enabled;
	}
	bool enabled() const
	{
		return m_enabled;
	}

private:
	iALogRedirectVTK(): m_enabled(true)
	{}
	iALogRedirectVTK(const iALogRedirectVTK &) = delete;
	void operator=(const iALogRedirectVTK &) = delete;

	bool m_enabled;
};

vtkStandardNewMacro(iALogRedirectVTK);

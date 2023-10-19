// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
		default                          : [[fallthrough]];
		case MESSAGE_TYPE_WARNING        : [[fallthrough]];
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

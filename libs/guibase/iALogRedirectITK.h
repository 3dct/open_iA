// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iALog.h>

#include <itkOutputWindow.h>

//! Redirects all log output from ITK to the LOG macro (i.e. iALog)
class iALogRedirectITK : public itk::OutputWindow
{
public:
	typedef iALogRedirectITK                Self;
	typedef itk::OutputWindow               Superclass;
	typedef itk::SmartPointer< Self >       Pointer;
	typedef itk::SmartPointer< const Self > ConstPointer;
	itkTypeMacro(iALogRedirectITK, itk::OutputWindow);

	static Pointer New()
	{
		if (!m_instance)
		{
			iALogRedirectITK::m_instance = new iALogRedirectITK;
			iALogRedirectITK::m_instance->SetPromptUser(false);
			iALogRedirectITK::m_instance->UnRegister();
		}
		return m_instance;
	}
	void logMsg(iALogLevel lvl, const char *t)
	{
		if (!m_enabled)
		{
			return;
		}
		LOG(lvl, QString("ITK %1").arg(t));
	}

	void DisplayDebugText(const char *t) override
	{
		logMsg(lvlDebug, t);
	}
	void DisplayErrorText(const char* t) override
	{
		logMsg(lvlError, t);
	}
	void DisplayGenericOutputText(const char* t) override
	{
		logMsg(lvlInfo, t);
	}
	void DisplayText(const char* t) override
	{
		logMsg(lvlInfo, t);
	}
	void DisplayWarningText(const char* t) override
	{
		logMsg(lvlWarn, t);
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
	static Pointer m_instance;
	bool m_enabled = true;
};

iALogRedirectITK::Pointer iALogRedirectITK::m_instance;

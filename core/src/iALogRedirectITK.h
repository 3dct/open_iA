/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <itkOutputWindow.h>

class iALogRedirectITK : public itk::OutputWindow
{
public:
	typedef iALogRedirectITK                Self;
	typedef itk::OutputWindow               Superclass;
	typedef itk::SmartPointer< Self >       Pointer;
	typedef itk::SmartPointer< const Self > ConstPointer;
	itkTypeMacro(iALogRedirectITK, itk::OutputWindow);

	Pointer New()
	{
		if (!m_instance)
		{
			iALogRedirectITK::m_instance = new iALogRedirectITK;
			iALogRedirectITK::m_instance->UnRegister();
		}
		return m_instance;
	}

	void DisplayDebugText(const char *t)
	{
		LOG(lvlDebug, QString("ITK %1").arg(t));
	}

	void DisplayErrorText(const char *t)
	{
		LOG(lvlError, QString("ITK %1").arg(t));
	}

	void DisplayGenericOutputText(const char *t)
	{
		LOG(lvlInfo, QString("ITK %1").arg(t));
	}

	void DisplayText(const char * t)
	{
		LOG(lvlInfo, QString("ITK %1").arg(t));
	}

	void DisplayWarningText(const char *t)
	{
		LOG(lvlWarn, QString("ITK %1").arg(t));
	}

	void SetPromptUser(bool /*arg*/)
	{}

	bool GetPromptUser() const
	{
		return false;
	}

	void PromptUserOn()
	{}

	void PromptUserOff()
	{}
private:
	static Pointer m_instance;
};

iALogRedirectITK::Pointer iALogRedirectITK::m_instance;
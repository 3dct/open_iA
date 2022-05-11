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

#include <QObject>

#include "iAguibase_export.h"

class iAMainWindow;
class iAMdiChild;

//! Base class for data from a module that needs to be "attached" to a single iAMdiChild window.
//! Should only be created through iAModuleInterface::CreateAttachment, as this takes care
//! of attaching it to the iAMdiChild properly.
class iAguibase_API iAModuleAttachmentToChild : public QObject
{
	Q_OBJECT

public:
	iAModuleAttachmentToChild( iAMainWindow * mainWnd, iAMdiChild * child ) : m_mainWnd(mainWnd), m_child(child) {}
	virtual ~iAModuleAttachmentToChild() {}
	iAMdiChild * getMdiChild() const { return m_child; }
signals:
	//! emit this signal if you want the attachment to be removed from the current mdichild
	void detach();
protected:
	iAMainWindow * m_mainWnd;
	iAMdiChild * m_child;
};

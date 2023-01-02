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

#include "iAguibase_export.h"

class iAMainWindow;
class iAMdiChild;

class QSettings;
class QString;

class iAguibase_API iATool
{
public:
	//! implementation (empty) in iAToolRegistry.cpp
	iATool(iAMainWindow* mainWnd, iAMdiChild* child);
	virtual ~iATool();
	//! load the state of the tool from the  given settings
	virtual void loadState(QSettings & projectFile, QString const & fileName); // TODO: replace QSettings with QVariantMap? or at least make const (can't currently because of beginGroup/endGroup
	//! save the current state of the tool, so that the current window can be restored from the stored data via the loadState method
	virtual void saveState(QSettings & projectFile, QString const & fileName);
	//CHECK NEWIO: maybe introduce 
	//! indicate whether this tool should be loaded only once the rendering of datasets has been fully initialized
	// virtual bool waitForRendering() const;
protected:
	iAMdiChild* m_child;
	iAMainWindow* m_mainWindow;
};

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

#include "iALog.h"
#include "iAModuleInterface.h"
#include "iAQMenuHelper.h"    // to make getOrAddSubmenu / addToMenuSorted available as before

#include <QVector>

class iAMainWindow;
class iAMdiChild;

//! Base class for a module interface.
//! A class derived from this class, and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the FeatureScout module needs to contain a class iAFeatureScoutModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class iAguibase_API iAGUIModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	iAGUIModuleInterface();
	virtual ~iAGUIModuleInterface();
	//! Called by the module dispatcher on module initialization. There should be no need to call this method from user code
	void SetMainWindow( iAMainWindow * mainWnd );
	//! Override to add references to the module in the core code, for example menu entries.
	virtual void Initialize() = 0;		// TODO: split up into GUI part and other?
	//! Override to store custom settings of this module; called when program is shut down.
	virtual void SaveSettings() const;

protected:
	//! Create a new result child, with a title made from the given title + the previous title of the active child.
	//! @deprecated. Use methods from iAMainWindow directly
	void PrepareResultChild( QString const & title);
	//! Create a new result child at the given index in the iAMdiChild list with the given title.
	//! @deprecated. Use methods from iAMainWindow directly
	void PrepareResultChild( int childInd, QString const & title );
	//! Set the currently active child as "current".
	//! @deprecated. Don't use m_mdichild mechanism, use activeMdiChild from iAMainWindow directly.
	void PrepareActiveChild();

	iAMainWindow * m_mainWnd;            //!< access to the main window
	//! "current" mdi child
	//! @deprecated use direct access via iAMainWindow methods
	iAMdiChild * m_mdiChild;
};

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

#include "iAbase_export.h"

#include <QObject>

//! Base class for a module interface.
//! A class derived from this class (or any class derived from it, see e.g. iAGUIModuleInterface),
//! and having a name in the form iA<ModuleName>ModuleInterface needs to be part of each module.
//! E.g. the FeatureScout module needs to contain a class iAFeatureScoutModuleInterface.
//! At least the Initialize method needs to be overriden in order to add the custom code of the module to open_iA.
//! This can either be the addition of some filters, or adding an entry directly to open_iA's toolbar or menu.
class iAbase_API iAModuleInterface : public QObject
{
	Q_OBJECT
public:
	virtual ~iAModuleInterface();
	//! Override to add references to the module in the core code, for example menu entries.
	virtual void Initialize() = 0;
	//! Override to store custom settings of this module; called when program is shut down.
	virtual void SaveSettings() const;
	//! Called whenever an MdiChild object is created. Override to react on this.

};

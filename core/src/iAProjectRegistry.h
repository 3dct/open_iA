/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include "iAGenericFactory.h"

#include <QMap>

class iAProjectBase;

//! For internal use in iAProjectRegistry only.
//! There should be no need to use this class directly; use REGISTER_PROJECT below!
using iAIProjectFactory = iAGenericFactory<iAProjectBase>;

class open_iA_Core_API iAProjectRegistry
{
public:
	//! Adds a given project type to the registry.
	template <typename ProjectType> static void addProject(QString const & projectIdentifier);
private:
	static QMap<QString, QSharedPointer<iAIProjectFactory> > m_projectTypes;
	iAProjectRegistry() =delete;	//!< iAProjectRegistry is meant to be used statically only, thus prevent creation of objects
};

template <typename ProjectType> using iAProjectFactory = iASpecificFactory<ProjectType, iAProjectBase>;

template <typename ProjectType>
void iAProjectRegistry::addProject(QString const & projectIdentifier)
{
	m_projectTypes.insert(projectIdentifier, QSharedPointer<iAIProjectFactory>(new iAProjectFactory<ProjectType>()));
}

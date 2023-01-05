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

#include "iAio_export.h"

#include "iADataSetType.h"
#include "iAFileIO.h"    // for iAFileIO::Operation

class QString;

using iAFileIOCreateFuncPtr = std::shared_ptr<iAFileIO>(*)();

//! Registry for file types (of type iAFileIO).
class iAio_API iAFileTypeRegistry final
{
public:
	//! Adds a given file type to the registry.
	static bool add(iAFileIOCreateFuncPtr c);

	//! Create a file I/O for the given extension
	static std::shared_ptr<iAFileIO> createIO(QString const & fileName, iAFileIO::Operation op);

	//! Retrieve list of file types for file open/save dialog
	static QString registeredFileTypes(iAFileIO::Operation op, iADataSetTypes allowedTypes = iADataSetType::All);

	//! Adds a default extension for a given file type
	static void addDefaultExtension(iADataSetType type, QString ext);

	//! Get the default extension for datasets of the given type
	static QString defaultExtension(iADataSetType type);

	//! Get the filter string for the default extension for a given dataset type.
	//! To be used in file open dialog, same string returned for that filetype as part of the result of registeredFileTypes
	static QString defaultExtFilterString(iADataSetType type);
private:
	iAFileTypeRegistry() = delete;  //!< class is meant to be used statically only, prevent creation of objects
};

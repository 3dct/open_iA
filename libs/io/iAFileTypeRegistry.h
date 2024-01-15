// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include "iADataSetType.h"
#include "iAFileIO.h"    // for iAFileIO::Operation

class QString;

using iAFileIOCreateFuncPtr = std::shared_ptr<iAFileIO>(*)();

//! Registry for file types (of type iAFileIO) with the file I/O framework of open_iA.
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

	//! Whether a default extension for datasets of the given type is available
	static bool defaultExtAvailable(iADataSetType type);

	//! Get the filter string for the default extension for a given dataset type.
	//! To be used in file open dialog, same string returned for that filetype as part of the result of registeredFileTypes
	static QString defaultExtFilterString(iADataSetType type);
private:
	iAFileTypeRegistry() = delete;  //!< class is meant to be used statically only, prevent creation of objects
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <QFlags>
#include <QStringList>

#include <string>

class QFileInfo;
class QString;

//! Given a base directory and a filename, return the complete, absolute filename
//! (basically, add the given baseDir as prefix, unless the given filename is an
//! a filename including a full path already)
iAbase_API QString MakeAbsolute(QString const & baseDir, QString const & fileName);
//! Given a base directory and a full filename (that is, including a path), make
//! that filename relativ to the base directory
iAbase_API QString MakeRelative(QString const & baseDir, QString const & fileName);

//! enum for filters to findFiles
enum FilesFolders
{
	Files = 0x1,
	Folders = 0x2
};

//! List files and/or folders inside of a given directory
//! @param directory the folder in which to search for files
//! @param filters a list of filters to apply (see QDirIterator nameFilters)
//! @param recurse whether to also search in sub-folders
//! @param filesOut reference to the container for the found filenames
//! @param filesFolders flags indicating whether to search for files, folders or both
iAbase_API void findFiles(QString const& directory, QStringList const& filters, bool recurse,
	QStringList & filesOut, QFlags<FilesFolders> filesFolders);

//! convert a given (utf-8 encoded) fileName in QString type to the (closest possible)
//! string representation in local encoding used for file names)
iAbase_API std::string getLocalEncodingFileName(QString const& fileName);

//! returns the full path of the given file along with the file's basename
//! (i.e., the file name including path, but excluding extension).
//! e.g. if the given QFileInfo points to C:/test/data.mhd, the function would return C:/test/data
iAbase_API QString pathFileBaseName(QFileInfo const& fi);

//! Determine parameters for (image) stacks from a given filename.
//!
//! Takes a filename, and checks the folder containing
//! this file for similarly named files; it determines a common prefix and suffix,
//! and the range of numbers contained in the part of the filename that varies.
//! If multiple files exist in the same folder, with different parts varying, then
//! the values for first of these will be contained in the returned variables.
//! example:
//!   content of folder: file1-1.tif  file1-2.tif  file1-3.tif file2-1.tif file2-2.tif
//!   filename:  file1-1.tif
//!   result: prefix="file1-", suffix=".tif", range=[1,3], digits=1
//!   (note that prefix="file", suffix="-1.tif", range=[1,2], digits=1 would also be a valid "solution")
//! @param[in]  fullFileName full file name (including path)
//! @param[out] prefix the prefix that all files which were determined to belong to the stack share
//! @param[out] suffix the suffix that all files which were determined to belong to the stack share
//! @param[out] range the minimum (index 0) and maximum (index 1) number occurring in the string part
//!     not shared between the filenames belonging to the stack
//! @param[out] digits the number of digits in the numbers of the filenames belonging to the stack
//!     (typically padded by zeros)
iAbase_API void determineStackParameters(QString const & fullFileName,
	QString & prefix, QString & suffix, int range[2], int & digits);

//! from the given string, filter all potentially unsafe/disallowed characters to return a valid filename
iAbase_API QString safeFileName(QString str);

//! For a given filename, test if it exists; if it does not exist,
//! try correcting the filename by using the given base path as file path instead of the one specified in the filename itself (if any)
//! @param fileName the file name to check for existence; can both be an absolute or a relative file name
//! @param basePath the folder to use as basis for checking where the file could be
//! @return the fileName parameter if the file that references exists, or a modified version using basePath as path if that exists
//! @throw std::runtime_error if the file could not be found even when trying correcting
iAbase_API QString tryFixFileName(QString const& fileName, QString const& basePath);

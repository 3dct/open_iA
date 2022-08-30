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

#include <QFlags>
#include <QStringList>

#include <string>

class QFileInfo;
class QString;

enum FilesFolders
{
	Files,
	Folders,
	FilesAndFolders
};

iAbase_API QString MakeAbsolute(QString const & baseDir, QString const & fileName);
iAbase_API QString MakeRelative(QString const & baseDir, QString const & fileName);

iAbase_API void FindFiles(QString const& directory, QStringList const& filters, bool recurse,
	QStringList & filesOut, QFlags<FilesFolders> filesFolders);

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

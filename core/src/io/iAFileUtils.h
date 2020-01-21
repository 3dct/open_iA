/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <QFlags>

#include <string>

class QString;
class QStringList;

enum FilesFolders
{
	Files,
	Folders,
	FilesAndFolders
};

open_iA_Core_API QString MakeAbsolute(QString const & baseDir, QString const & fileName);
open_iA_Core_API QString MakeRelative(QString const & baseDir, QString const & fileName);

open_iA_Core_API void FindFiles(QString const & directory, QStringList const & filters, bool recurse,
	QStringList & filesOut, QFlags<FilesFolders> filesFolders);

open_iA_Core_API std::string getLocalEncodingFileName(QString const & fileName);

open_iA_Core_API QString fileNameOnly(QString const & fileName);

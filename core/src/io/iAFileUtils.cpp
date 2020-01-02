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
#include "iAFileUtils.h"

#include "iAConsole.h"

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QString>

QString MakeAbsolute(QString const & baseDir, QString const & fileName)
{
#if _WIN32
	if (fileName.contains(":"))
#else
	if (fileName.startsWith("/"))
#endif
	{
		return fileName;
	}
	QDir dir(baseDir);
	// TODO : use canonicalFilePath here? drawback: empty if file doesn't exist!
	return dir.absoluteFilePath(fileName);
}

QString MakeRelative(QString const & baseDir, QString const & fileName)
{
	QDir dir(baseDir);
	return dir.relativeFilePath(fileName);
}

void FindFiles(QString const & directory, QStringList const & nameFilters, bool recurse,
	QStringList & filesOut, QFlags<FilesFolders> filesFolders)
{
	QDir::Filters filters = QDir::Files;
	if (recurse || filesFolders.testFlag(Folders))
		filters = QDir::Files | QDir::AllDirs;
	QDirIterator::IteratorFlags flags = (recurse) ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
	QDirIterator it(directory, nameFilters, filters, flags);
	// TODO:
	//   - when folders are considered, they seem to be listed twice
	//   - "natural" sort order
	while (it.hasNext())
	{
		QString fileName = it.next();
		if (fileName == "." || fileName == ".." ||
			(QFileInfo(fileName).isDir() && !filesFolders.testFlag(Folders)) )
			continue;
		filesOut.append(fileName);
	}
}

std::string getLocalEncodingFileName(QString const & fileName)
{
	QByteArray fileNameEncoded = fileName.toLocal8Bit();
	if (fileNameEncoded.contains('?'))
	{
		DEBUG_LOG(QString("File name '%1' not convertible to a system encoding string. "
			"Please specify a filename without special characters!").arg(fileName));
		return std::string();
	}
	return std::string(fileNameEncoded.constData());
}

QString fileNameOnly(QString const & f)
{
	return QFileInfo(f).fileName();
}

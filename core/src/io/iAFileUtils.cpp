/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFileUtils.h"

#include "iAConsole.h"
#include "iAStringHelper.h"

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
		{
			continue;
		}
		filesOut.append(fileName);
	}
	filesOut.sort();
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

QString pathFileBaseName(QFileInfo const& fi)
{
	return fi.absolutePath() + "/" + fi.completeBaseName();
}

void determineStackParameters(QString const& fullFileName,
	QString& prefix, QString& suffix, int range[2], int& digits)
{
	QFileInfo fi(fullFileName);
	QDir dir(fi.absolutePath());
	QStringList nameFilters;
	nameFilters << "*." + fi.suffix();
	QFileInfoList imgFiles = dir.entryInfoList(nameFilters);
	prefix = fi.absoluteFilePath();
	suffix = fi.absoluteFilePath();
	for (QFileInfo imgFileInfo : imgFiles)
	{
		QString imgFileName = imgFileInfo.absoluteFilePath();
		if (imgFileName == fi.absoluteFilePath())
		{
			continue;
		}
		auto newPrefix = greatestCommonPrefix(prefix, imgFileName);
		auto newSuffix = greatestCommonSuffix(suffix, imgFileName);
		//DEBUG_LOG(QString("  File %1: new prefix=%2, new suffix=%3").arg(imgFileName).arg(newPrefix).arg(newSuffix));
		auto differentPartLength = imgFileName.length() - newPrefix.length() - newSuffix.length();
		auto differentPart = imgFileName.mid(newPrefix.length(), differentPartLength);
		bool ok;
		/* auto differentSuffixNr = */ differentPart.toInt(&ok);
		if (ok)
		{
			prefix = newPrefix;
			suffix = newSuffix;
		}
		//else
		//{
		//	DEBUG_LOG(QString("    Skipping: Part differing (%1) from chosen file (%2) is not a number!").arg(differentPart).arg(f));
		//}
	}
	//DEBUG_LOG(QString("FINAL prefix=%1, suffix=%2").arg(m_fileNamesBase).arg(m_extension));
	digits = fi.absoluteFilePath().length();
	range[0] = std::numeric_limits<int>::max();
	range[1] = std::numeric_limits<int>::min();
	if (prefix == fi.absoluteFilePath() || suffix == fi.absoluteFilePath())
	{
		//DEBUG_LOG("Automatic determination of prefix and suffix failed, could not determine any valid range of which the given filename is a part!");
		// fallback: set full filename as base, and extension as suffix
		prefix = fi.absoluteFilePath().left(fi.absoluteFilePath().length() - fi.suffix().length() - 1);
		suffix = "." + fi.suffix();
		digits = 0;
	}
	else
	{
		// determine index range:
		//DEBUG_LOG("Determine index range:");
		for (QFileInfo imgFileInfo : imgFiles)
		{
			QString imgFileName = imgFileInfo.absoluteFilePath();
			int len = imgFileName.length() - prefix.length() - suffix.length();
			if (!imgFileName.startsWith(prefix) || !imgFileName.endsWith(suffix) || len <= 0)
			{
				//DEBUG_LOG(QString("  File %1: does not match prefix(%2) or suffix(%3); or length of extracted number (%4) string would be <= 0").arg(imgFileName).arg(m_fileNamesBase).arg(m_extension).arg(len));
				continue;
			}
			QString numStr = imgFileName.mid(prefix.length(), len);
			//DEBUG_LOG(QString("  File %1: numStr=%2 (start=%3, len=%4)").arg(imgFileName).arg(numStr).arg(prefix.length()).arg(len));
			digits = std::min(numStr.length(), digits);
			bool ok;
			int num = numStr.toInt(&ok);
			if (!ok)
			{
				//DEBUG_LOG(QString("    Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
				continue;
			}
			if (num < range[0])
			{
				range[0] = num;
			}
			if (num > range[1])
			{
				range[1] = num;
			}
			//DEBUG_LOG(QString("  New range: %1-%2").arg(range[0]).arg(range[1]));
		}
		// Check if all files in range exist:
		for (int val = range[0]; val <= range[1]; ++val)
		{
			QString filename(prefix + QString::number(val).rightJustified(digits, '0') + suffix);
			if (!QFile::exists(filename))
			{
				DEBUG_LOG(QString("NOTE: Filename '%1' would be in determined range, but it does not exist! "
					"Loading with the determined min/max values will fail!").arg(filename));
			}
		}
	}
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFileUtils.h"

#include "iALog.h"
#include "iAStringHelper.h"

#include <QCollator>
#include <QDirIterator>
#include <QRegularExpression>
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

void findFiles(QString const & directory, QStringList const & nameFilters, bool recurse,
	QStringList & filesOut, QFlags<FilesFolders> filesFolders)
{
	QDir::Filters filters = QDir::Files;
	if (recurse || filesFolders.testFlag(Folders))
	{
		filters = QDir::Files | QDir::AllDirs;
	}
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
		LOG(lvlWarn, QString("File name '%1' not convertible to a system encoding string. "
			"Please specify a filename without special characters!").arg(fileName));
		return std::string();
	}
	return std::string(fileNameEncoded.constData());
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
		//LOG(lvlInfo, QString("  File %1: new prefix=%2, new suffix=%3").arg(imgFileName).arg(newPrefix).arg(newSuffix));
		auto differentPartLength = imgFileName.length() - newPrefix.length() - newSuffix.length();
		auto differentPart = imgFileName.mid(newPrefix.length(), differentPartLength);
		bool ok;
		/* auto differentSuffixNr = */ differentPart.toInt(&ok);
		if (ok)
		{
			prefix = newPrefix;
			suffix = newSuffix;
		}
		// detect invalid / ambiguous configurations?
		// a non-shared part is not a number (but skip files with completely different base?):
		//else
		//{
		//	LOG(lvlWarn, QString("    Skipping: Part differing (%1) from chosen file (%2) is not a number!").arg(differentPart).arg(f));
		//}
		// filename of first file is completely contained in the current file:
		//if (newPrefix == fi.absoluteFilePath())
		//{
		//	LOG(lvlWarn, QString("    Full selected filename is contained in another file name")
		//}
	}
	//LOG(lvlInfo, QString("FINAL prefix=%1, suffix=%2").arg(m_fileNamesBase).arg(m_extension));
	digits = fi.absoluteFilePath().length();
	range[0] = std::numeric_limits<int>::max();
	range[1] = std::numeric_limits<int>::min();
	if (prefix == fi.absoluteFilePath() || suffix == fi.absoluteFilePath())
	{
		//LOG(lvlWarn, "Automatic determination of prefix and suffix failed, could not determine any valid range of which the given filename is a part!");
		// fallback: set full filename as base, and extension as suffix
		prefix = fi.absoluteFilePath().left(fi.absoluteFilePath().length() - fi.suffix().length() - 1);
		suffix = "." + fi.suffix();
		digits = 0;
	}
	else
	{
		// determine index range:
		//LOG(lvlInfo, "Determine index range:");
		for (QFileInfo imgFileInfo : imgFiles)
		{
			QString imgFileName = imgFileInfo.absoluteFilePath();
			int len = imgFileName.length() - prefix.length() - suffix.length();
			if (!imgFileName.startsWith(prefix) || !imgFileName.endsWith(suffix) || len <= 0)
			{
				//LOG(lvlInfo, QString("  File %1: does not match prefix(%2) or suffix(%3); or length of extracted number (%4) string would be <= 0").arg(imgFileName).arg(m_fileNamesBase).arg(m_extension).arg(len));
				continue;
			}
			QString numStr = imgFileName.mid(prefix.length(), len);
			//LOG(lvlInfo, QString("  File %1: numStr=%2 (start=%3, len=%4)").arg(imgFileName).arg(numStr).arg(prefix.length()).arg(len));
			digits = std::min(static_cast<int>(numStr.length()), digits);
			bool ok;
			int num = numStr.toInt(&ok);
			if (!ok)
			{
				//LOG(lvlInfo, QString("    Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
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
			//LOG(lvlInfo, QString("  New range: %1-%2").arg(range[0]).arg(range[1]));
		}
		// Check if all files in range exist:
		for (int val = range[0]; val <= range[1]; ++val)
		{
			QString filename(prefix + QString("%1").arg(val, digits, 10, QChar('0')) + suffix);
			if (!QFile::exists(filename))
			{
				LOG(lvlWarn, QString("NOTE: Filename '%1' would be in determined range, but it does not exist! "
					"Loading with the determined min/max values will fail!").arg(filename));
			}
		}
	}
}

QString safeFileName(QString str)
{
	return str.replace(QRegularExpression("[\\\\/:;*?!\"'`<>{}|#%&$@+= ]"), "_");
}

QString tryFixFileName(QString const& fileName, QString const& basePath)
{
	auto rawFileName = fileName;
	if (!QFile::exists(rawFileName))
	{
		if ((rawFileName.lastIndexOf("\\") == -1) && (rawFileName.lastIndexOf("/") == -1))
		{
			rawFileName = basePath + "/" + rawFileName;
		}
		else if (rawFileName.lastIndexOf("\\") > 0)
		{
			rawFileName = basePath + "/" + rawFileName.section('\\', -1);
		}
		else if (rawFileName.lastIndexOf("/") > 0)
		{
			rawFileName = basePath + "/" + rawFileName.section('/', -1);
		}
	}
	if (!QFile::exists(rawFileName))
	{
		throw std::runtime_error(QString("Raw file name is wrong, file (%1) does not exist").arg(fileName).toStdString());
	}
	return rawFileName;
}

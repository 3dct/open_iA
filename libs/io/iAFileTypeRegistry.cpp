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
#include "iAFileTypeRegistry.h"

#include <QFileInfo>

std::vector<iAFileIOCreateFuncPtr> iAFileTypeRegistry::m_fileIOs;
QMap<QString, size_t> iAFileTypeRegistry::m_fileTypes;

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileName)
{
	QFileInfo fi(fileName);
	// special handling for directory ? TLGICT-loader... -> fi.isDir();
	auto ext = fi.suffix().toLower();
	if (m_fileTypes.contains(ext))
	{
		return m_fileIOs[m_fileTypes[ext]]();
	}
	else
	{
		LOG(lvlWarn,
			QString("Failed to load %1: There is no handler registered files with suffix '%2'")
			.arg(fileName)
			.arg(fi.suffix()));
		return std::shared_ptr<iAFileIO>();
	}
	// for file formats that support multiple dataset types: check if an allowed type was loaded?
	// BUT currently no such format supported
}

QString iAFileTypeRegistry::registeredFileTypes(iAFileIO::Operation op, iADataSetTypes allowedTypes)
{
	QStringList allExtensions;
	QString singleTypes;
	for (auto ioFactory : m_fileIOs)  // all registered file types
	{
		auto io = ioFactory();
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
		if ( (io->supportedDataSetTypes(op) & allowedTypes) == 0 )
#else
		if (!io->supportedDataSetTypes(op).testAnyFlags(allowedTypes))
#endif
		{   // current I/O does not support any of the allowed types
			continue;
		}
		auto extCpy = io->extensions();
		for (auto & ext: extCpy)
		{
			ext = "*." + ext;
		}
		singleTypes += QString("%1 (%2);;").arg(io->name()).arg(extCpy.join(" "));
		allExtensions.append(extCpy);
	}
	if (singleTypes.isEmpty())
	{
		LOG(lvlWarn, "No matching registered file types found!");
		return ";;";
	}
	return QString("Any supported format (%1);;").arg(allExtensions.join(" ")) + singleTypes;
}

bool iAFileTypeRegistry::addFileType(iAFileIOCreateFuncPtr c)
{
	m_fileIOs.push_back(c);
	auto io = c();
	for (auto extension : io->extensions())
	{
		auto lowerExt = extension.toLower();
		if (m_fileTypes.contains(extension))
		{
			LOG(lvlWarn, QString("File IO %1 tries to add a handler for file extension %2, already registered to file IO %3!")
				.arg(io->name())
				.arg(lowerExt)
				.arg(m_fileIOs[m_fileTypes[lowerExt]]()->name()));
		}
		m_fileTypes.insert(lowerExt, m_fileIOs.size() - 1);
	}
	return true;
}

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

#include <QMap>

#include <vector>

namespace
{// if the data structures here would be members of iAFileTypeRegistry, we would run into the "Static Initialization Order Fiasco"!
	std::vector<iAFileIOCreateFuncPtr> & fileIOs()
	{
		static std::vector<iAFileIOCreateFuncPtr> fileios;
		return fileios;
	}
	QMap<iADataSetType, QString>& defaultExtensions()
	{
		static QMap<iADataSetType, QString> defaultExts;
		return defaultExts;
	}
}

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileName, iAFileIO::Operation op)
{
	QFileInfo fi(fileName);
	// special handling for directory ? TLGICT-loader... -> fi.isDir();
	auto fileExt = fi.suffix().toLower();
	// check for whether we have a two-parts suffix (such as .nii.gz or the like)
	// we need to find the second to last "." and extract the extension from its position
	auto fileExtFull = fileExt;
	auto secondLastDotPos = fi.fileName().lastIndexOf(".", -(fileExt.size() + 2));
	if (secondLastDotPos != -1)
	{
		fileExtFull = fi.fileName().right(fi.fileName().size() - (secondLastDotPos+1));
	}

	for (auto c : fileIOs())
	{
		auto io = c();
		for (auto ioExt : io->extensions())
		{
			if ( (fileExt == ioExt || fileExtFull == ioExt) &&
				!io->supportedDataSetTypes(op))  // QFlags::operator! checks for empty, i.e. value 0
			{
				return io;
			}
		}
	}
	LOG(lvlWarn,
		QString("Failed to load %1: There is no handler registered files with suffix '%2'")
		.arg(fileName)
		.arg(fi.suffix()));
	return std::shared_ptr<iAFileIO>();
}

QString iAFileTypeRegistry::registeredFileTypes(iAFileIO::Operation op, iADataSetTypes allowedTypes)
{
	QStringList allExtensions;
	QString singleTypes;
	for (auto ioFactory : fileIOs())  // all registered file types
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
		singleTypes += io->filterString() + ";;";
		allExtensions.append(io->filterExtensions());
	}
	if (singleTypes.isEmpty())
	{
		LOG(lvlWarn, "No matching registered file types found!");
		return ";;";
	}
	return QString("Any supported format (%1);;").arg(allExtensions.join(" ")) + singleTypes;
}

bool iAFileTypeRegistry::add(iAFileIOCreateFuncPtr c)
{
	fileIOs().push_back(c);
	return true;
}

void iAFileTypeRegistry::addDefaultExtension(iADataSetType type, QString ext)
{
	if (defaultExtensions().contains(type))
	{
		LOG(lvlWarn, QString("For type %1, when trying to register extension %2 as default:"
			" There is already a default extension registered (%3)!").arg(static_cast<int>(type))
			.arg(ext)
			.arg(defaultExtensions()[type]));
	}
	defaultExtensions().insert(type, ext);

}

QString iAFileTypeRegistry::defaultExtension(iADataSetType type)
{
	if (!defaultExtensions().contains(type))
	{
		LOG(lvlError, QString("No default extension registered for dataset type %1!").arg(static_cast<int>(type)));
		return {};
	}
	return defaultExtensions()[type];
}

QString iAFileTypeRegistry::defaultExtFilterString(iADataSetType type)
{
	if (!defaultExtensions().contains(type))
	{
		LOG(lvlError, QString("No default extension registered for dataset type %1!").arg(static_cast<int>(type)));
		return {};
	}
	auto ext = defaultExtensions()[type];
	for (auto ioFactory : fileIOs())  // all registered file types
	{
		auto io = ioFactory();
		if (io->extensions().contains(ext))
		{
			return io->filterString();
		}
	}
	LOG(lvlError, QString("No File I/O registered for default extension '%1'!").arg(ext));
	return {};
}

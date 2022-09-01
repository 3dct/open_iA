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
}

std::shared_ptr<iAFileIO> iAFileTypeRegistry::createIO(QString const& fileName)
{
	QFileInfo fi(fileName);
	// special handling for directory ? TLGICT-loader... -> fi.isDir();
	auto fileExt = fi.suffix().toLower();

	for (auto c : fileIOs())
	{
		auto io = c();
		for (auto ioExt : io->extensions())
		{
			if (fileExt == ioExt)
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

bool iAFileTypeRegistry::add(iAFileIOCreateFuncPtr c)
{
	fileIOs().push_back(c);
	return true;
}

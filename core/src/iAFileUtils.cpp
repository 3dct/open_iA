/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#include "pch.h"
#include "iAFileUtils.h"

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
	// TODO : use canonicalFilePath here? drawback: empty if file doesn't exist!
	return baseDir + "/" + fileName;
}

QString MakeRelative(QString const & baseDir,  QString const & fileName)
{
	if (fileName.startsWith(baseDir))
	{                                                               // for '/'
		return fileName.right(fileName.length() - baseDir.length() - 1);
	}
	return fileName;
}

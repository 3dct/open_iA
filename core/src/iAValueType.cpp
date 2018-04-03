/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAValueType.h"

#include "iAConsole.h"

#include <QString>

namespace
{
	// Value Types:
	const QString ContinuousStr("Continuous");
	const QString DiscreteStr("Discrete");
	const QString CategoricalStr("Categorical");
	const QString StringStr("String");
	const QString BooleanStr("Boolean");
	const QString TextStr("Text(file)");
	const QString FileNameOpenStr("File name (read)");
	const QString FileNamesOpenStr("File names (read)");
	const QString FileNameSaveStr("File name (write)");
	const QString FolderStr("Folder");
	const QString UnknownStr("Unknown");
}


iAValueType Str2ValueType(QString const & str)
{
	if (str == ContinuousStr)
	{
		return iAValueType::Continuous;
	}
	else if (str == DiscreteStr)
	{
		return iAValueType::Discrete;
	}
	else if (str == CategoricalStr)
	{
		return iAValueType::Categorical;
	}
	else if (str == BooleanStr)
	{
		return iAValueType::Boolean;
	}
	else if (str == StringStr)
	{
		return iAValueType::String;
	}
	else if (str == TextStr)
	{
		return iAValueType::Text;
	}
	else if (str == FileNameOpenStr)
	{
		return iAValueType::FileNameOpen;
	}
	else if (str == FileNamesOpenStr)
	{
		return iAValueType::FileNamesOpen;
	}
	else if (str == FileNameSaveStr)
	{
		return iAValueType::FileNameSave;
	}
	else if (str == FolderStr)
	{
		return iAValueType::Folder;
	}
	else
	{
		DEBUG_LOG(QString("Unknown value type '%1'\n").arg(str));
		return iAValueType::Invalid;
	}
}

QString ValueType2Str(iAValueType type)
{
	switch (type)
	{
	case iAValueType::Continuous:
		return ContinuousStr;
	case iAValueType::Discrete:
		return DiscreteStr;
	case iAValueType::Categorical:
		return CategoricalStr;
	case iAValueType::Boolean:
		return BooleanStr;
	case iAValueType::String:
		return StringStr;
	case iAValueType::Text:
		return TextStr;
	case iAValueType::FileNameOpen:
		return FileNameOpenStr;
	case iAValueType::FileNamesOpen:
		return FileNamesOpenStr;
	case iAValueType::FileNameSave:
		return FileNameSaveStr;
	case iAValueType::Folder:
		return FolderStr;
	default:
		return UnknownStr;
	}
}
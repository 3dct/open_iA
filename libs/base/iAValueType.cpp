// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAValueType.h"

#include "iALog.h"
#include "iAStringHelper.h"    // for joinNumbersAsString

#include <QColor>
#include <QString>
#include <QVariant>

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
	const QString ColorStr("Color");
	const QString Vector2Str("2-component double vector");
	const QString Vector3Str("3-component double vector");
	const QString Vector2iStr("2-component integer vector");
	const QString Vector3iStr("3-component integer vector");
	const QString UnknownValueTypeStr("Unknown");
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
	else if (str == ColorStr)
	{
		return iAValueType::Color;
	}
	else if (str == Vector2Str)
	{
		return iAValueType::Vector2;
	}
	else if (str == Vector3Str)
	{
		return iAValueType::Vector3;
	}
	else if (str == Vector2iStr)
	{
		return iAValueType::Vector2i;
	}
	else if (str == Vector3iStr)
	{
		return iAValueType::Vector3i;
	}
	else
	{
		LOG(lvlWarn, QString("Unknown value type '%1'\n").arg(str));
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
	case iAValueType::Color:
		return ColorStr;
	case iAValueType::Vector2:
		return Vector2Str;
	case iAValueType::Vector3:
		return Vector3Str;
	case iAValueType::Vector2i:
		return Vector2iStr;
	case iAValueType::Vector3i:
		return Vector3iStr;
	default:
		return UnknownValueTypeStr;
	}
}

QString variantValueToString(iAValueType valueType, QVariant value)
{
	switch (valueType)
	{
	case iAValueType::Boolean: return value.toBool() ? "yes" : "no";
/*
	case iAValueType::Vector2i:
		[[fallthrough]];
	case iAValueType::Vector3i: return joinNumbersAsString(value.value<QVector<int>>(), ", ");
	case iAValueType::Vector2:
		[[fallthrough]];
	case iAValueType::Vector3: return joinNumbersAsString(value.value<QVector<double>>(), ", ");
*/
	default: return value.toString();
	}
}

QColor variantToColor(QVariant const& v)
{
	return QColor(v.toString());
}

QVariant colorToVariant(QColor const& c)
{
	return c.name(QColor::HexArgb);
}

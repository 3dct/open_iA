/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include "open_iA_Core_export.h"

#include <QVector>

class QString;
class QStringList;

//! split a string at the space characters, while correctly treating quoted elements
//!
//! Example: the string '"a rabbit" and "a horse"' would be split into three elements:
//!     "a rabbit", "and", "a horse" (the quotes are stripped from the elements).
//!     Note that only the double-quote character is considered as a quote by this function.
//! @param str the string to split
//! @return a list of strings split up at the whitespaces
open_iA_Core_API QStringList SplitPossiblyQuotedString(QString const & str);

//! Convert a given string representation to a double vector with three elements
bool Str2Vec3D(QString const & str, double vec[3]);

//! Convert a given double vector with three elements to a string representation
QString Vec3D2String(double* vec);

//! Pads or truncates the given string to the given size.
//!
//! If the string given in name is longer than the specified size, the string is truncated
//! to size-2 and ".." is appended, otherwise it is filled with spaces to be exactly size long
//! @param str the string to be padded or truncated
//! @param size the size that the return string should have
//! @return a string of exactly the length size, a padded or truncated form of the given name
QString PadOrTruncate(QString const & str, int size);

//! join a vector of numeric types T to string, using the given string as item separator
//!
//! works similar t QString::join, but on arbitrary QVector types which can be converted to QString
//! via QString::number.
//! @param vec the vector to be joined
//! @param joinStr the string to be used in between the elements of the string
//! @return a string joining all elements of the given vector together
template <typename T>
QString Join(QVector<T> const & vec, QString const & joinStr)
{
	QString result;
	bool first = true;
	for (T elem : vec)
	{
		if (!first)
			result += joinStr;
		else
			first = false;
		result += QString::number(elem);
	}
	return result;
}
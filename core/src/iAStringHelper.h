/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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

open_iA_Core_API QString QuoteString(QString const & str);

//! Convert a given string representation to a double vector with three elements
open_iA_Core_API bool Str2Vec3D(QString const & str, double vec[3]);

//! Convert a given double vector with three elements to a string representation
open_iA_Core_API QString Vec3D2String(double* vec);

//! Pads or truncates the given string to the given size.
//!
//! If the string given in name is longer than the specified size, the string is truncated
//! to size-2 and ".." is appended, otherwise it is filled with spaces to be exactly size long
//! @param str the string to be padded or truncated
//! @param size the size that the return string should have
//! @return a string of exactly the given size, padded or truncated from the given name
open_iA_Core_API QString PadOrTruncate(QString const & str, int size);

//! strip HTML tags from the given string
//! @param html a string potentially containing HTML tags
//! @return the input string with all HTML tags (<xyz>, </xyz>, <xyz/>) removed
open_iA_Core_API QString StripHTML(QString const & html);

//! returns the value converted to string, with units (K, M, G, T, P) applied for every 10³ factor over 1000
open_iA_Core_API QString DblToStringWithUnits(double value);

//! join an iterable collection of numeric elements to a string
//!
//! works similar to QString::join, but on arbitrary iterable collection types
//! containing items which can be converted to QString via QString::number.
//! @param vec the collection of elements to be joined
//! @param joinStr the string to be used in between the elements of the string
//! @return a string joining all elements of the given collection together
template <template <typename...> class Container, typename Element>
QString Join(Container<Element> const & vec, QString const & joinStr)
{
	QString result;
	bool first = true;
	for (Element elem : vec)
	{
		if (!first)
			result += joinStr;
		else
			first = false;
		result += QString::number(elem);
	}
	return result;
}

//! find the greatest common prefix of the two given strings
//! example: str1 ="BaseMethod", str2="BaseMember"
//!     result: "BaseMe"
open_iA_Core_API QString GreatestCommonPrefix(QString const & str1, QString const & str2);

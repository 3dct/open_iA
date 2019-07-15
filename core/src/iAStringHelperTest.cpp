/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iASimpleTester.h"
#include "iAStringHelper.h"

#include <QStringList>

std::ostream& operator<<(std::ostream& o, const QString & s)
{
	o << s.toStdString();
	return o;
}

BEGIN_TEST
	// special case: empty quote at beginning
	QStringList split = splitPossiblyQuotedString("\"\" a b");
	TestEqual(split.size(), 3);
	TestEqual(split.at(0), QString(""));
	TestEqual(split.at(1), QString("a"));
	TestEqual(split.at(2), QString("b"));

	// special case: no space between two quoted strings:
	QStringList split2 = splitPossiblyQuotedString("\"a\"\"b\"");
	TestEqual(split2.size(), 2);
	TestEqual(split2.at(0), QString("a"));
	TestEqual(split2.at(1), QString("b"));

	// special case: empty quote at end
	QStringList split3 = splitPossiblyQuotedString("a b \"\"");
	TestEqual(split3.size(), 3);
	TestEqual(split3.at(0), QString("a"));
	TestEqual(split3.at(1), QString("b"));
	TestEqual(split3.at(2), QString(""));

	// special case: space at end of quoted string:
	QStringList split4 = splitPossiblyQuotedString("a \"b     \"");
	TestEqual(split4.size(), 2);
	TestEqual(split4.at(0), QString("a"));
	TestEqual(split4.at(1), QString("b     "));

	// "normal" case
	QStringList split5 = splitPossiblyQuotedString("a \"b c\" d");
	TestEqual(split5.size(), 3);
	TestEqual(split5.at(0), QString("a"));
	TestEqual(split5.at(1), QString("b c"));
	TestEqual(split5.at(2), QString("d"));

	// special case: empty quote in the middle
	QStringList split6 = splitPossiblyQuotedString("a \"\" b");
	TestEqual(split6.size(), 3);
	TestEqual(split6.at(0), QString("a"));
	TestEqual(split6.at(1), QString(""));
	TestEqual(split6.at(2), QString("b"));
END_TEST

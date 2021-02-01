/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iASimpleTester.h"
#include "iAStringHelper.h"

#include <QStringList>

std::ostream& operator<<(std::ostream& o, const QString & s)
{
	o << s.toStdString();
	return o;
}

BEGIN_TEST

	// TEST splitPossiblyQuotedString

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


	// TEST dblToStringWithUnits
	
	TestEqual(dblToStringWithUnits(0.000000000000001234), QString("1.23f"));
	TestEqual(dblToStringWithUnits(0.00000001234), QString("12.3n"));
	TestEqual(dblToStringWithUnits(0.000123), QString("123µ"));
	TestEqual(dblToStringWithUnits(0.123), QString("123m"));
	TestEqual(dblToStringWithUnits(1.423), QString("1.42"));
	TestEqual(dblToStringWithUnits(1.5353), QString("1.54"));
	TestEqual(dblToStringWithUnits(12.32), QString("12.3"));
	TestEqual(dblToStringWithUnits(323), QString("323"));
	TestEqual(dblToStringWithUnits(0.323, 100), QString("0.32"));
	TestEqual(dblToStringWithUnits(0.09, 100), QString("90.0m"));
	TestEqual(dblToStringWithUnits(0.009, 10), QString("9.00m"));
	TestEqual(dblToStringWithUnits(0.000323, 100), QString("0.32m"));
	TestEqual(dblToStringWithUnits(0.000323), QString("323µ"));
	TestEqual(dblToStringWithUnits(5323), QString("5.32K"));
	TestEqual(dblToStringWithUnits(6306403), QString("6.31M"));
	TestEqual(dblToStringWithUnits(92314300), QString("92.3M"));


	// TEST requiredDigits
	TestEqual(requiredDigits(0.00000002), 1);
	TestEqual(requiredDigits(0.25674), 1);
	TestEqual(requiredDigits(0.2), 1);
	TestEqual(requiredDigits(9.9999), 1);
	TestEqual(requiredDigits(10), 2);

	TestEqual(requiredDigits(1000000), 7);
	TestEqual(requiredDigits(9999999.99), 7);
	TestEqual(requiredDigits(123456789), 9);


	// TEST digitsAfterComma

	TestEqual(digitsAfterComma(10.5) , 0);  // -> debatable whether that's "correct", but that's the way it's currently implemented
	TestEqual(digitsAfterComma(9.95) , 1);
	TestEqual(digitsAfterComma(2.5)  , 1);
	TestEqual(digitsAfterComma(1)    , 0);
	TestEqual(digitsAfterComma(0.02) , 2);
	TestEqual(digitsAfterComma(0.025), 3);
	TestEqual(digitsAfterComma(0.001), 3);
	TestEqual(digitsAfterComma(0.0011), 4);


	std::vector<double> vecDbl = { 0.5, 1.222, 5, 3.33443 };
	TestEqual(QString("0.5, 1.222, 5, 3.33443"), joinNumbersAsString(vecDbl, ", "));
	TestEqual(QString("0.5,1.2,5,3.3"), joinAsString(vecDbl, ",", [](double v) { return QString::number(v, 'g', 2);  }));

	TestEqual(0, greatestCommonPrefixLength("", "TEST"));
	TestEqual(0, greatestCommonPrefixLength("TEST", ""));
	TestEqual(0, greatestCommonPrefixLength("TEST", "test"));
	TestEqual(3, greatestCommonPrefixLength("abcDEFGHIJ", "abcdeFGHIJ"));
	TestEqual(5, greatestCommonSuffixLength("abcDEFGHIJ", "abcdeFGHIJ"));
	TestEqual(4, greatestCommonPrefixLength("TEST", "TEST"));
END_TEST

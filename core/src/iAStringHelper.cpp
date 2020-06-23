/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAStringHelper.h"

#include <QRegularExpression>


QStringList splitPossiblyQuotedString(QString const & str)
{
	// TODO : rewrite so that it can cope with multiply quoted strings
	//     (where quotes are escaped by \" )
	QStringList result;
	QRegularExpression exp("\\s*([^\"]\\S*|\".*?\")\\s*");
	int offset = 0;
	QRegularExpressionMatch match = exp.match(str, offset);
	while (match.hasMatch())
	{
		QString argument = match.captured(1);
		if (argument.startsWith("\"") && argument.endsWith("\""))
		{
			argument = argument.mid(1, argument.length() - 2);
		}
		result.append(argument);
		offset = match.capturedEnd(0);
		match = exp.match(str, offset);
	}
	return result;
}

QString quoteString(QString const & str)
{
	QString result = str;
	result.replace("\"", "\\\"");
	return "\"" + result + "\"";
}

QString padOrTruncate(QString const & str, int size)
{
	return (str.length() > size) ?
		(str.left(size - 2) + "..") :
		str.leftJustified(size, ' ');
}

QString stripHTML(QString const & html)
{
	QString result(html);
	return result.remove(QRegExp("<[^>]*>"));
}

namespace
{
	// Large Values:
	const double OneKilo = 1000;
	const double OneMega = OneKilo * OneKilo;
	const double OneGiga = OneKilo * OneMega;
	const double OneTera = OneKilo * OneGiga;
	const double OnePeta = OneKilo * OneTera;
	const size_t UnitCount = 5;
	const double UnitPrefixLargeVal[UnitCount] = { OnePeta, OneTera, OneGiga, OneMega, OneKilo };
	const char UnitPrefixLarge[UnitCount] = { 'P', 'T', 'G', 'M', 'K' };

	// Small Values:
	const double OneMilli = 0.001;
	const double OneMicro = OneMilli * OneMilli;
	const double OneNano  = OneMilli * OneMicro;
	const double OnePico  = OneMilli * OneNano;
	const double OneFemto = OneMilli * OnePico;
	const double UnitPrefixSmallVal[UnitCount] = { OneFemto, OnePico, OneNano, OneMicro, OneMilli };
	const char UnitPrefixSmall[UnitCount] = { 'f', 'p', 'n', 'µ', 'm' };
}

QString dblToStringWithUnits(double value)
{
	// values between -1 and +1:
	if (value >= -1.0 && value < 1.0)
	{
		for (size_t u = 0; u < UnitCount; ++u)
		{
			if (value < UnitPrefixSmallVal[u]*OneKilo)
			{
				return QString::number(value * UnitPrefixLargeVal[u], 'f',
					(value < 10 * UnitPrefixSmallVal[u]) ? 2 : ((value < 100 * UnitPrefixSmallVal[u]) ? 1 : 0)) + UnitPrefixSmall[u];
			}
		}
		return QString::number(value, 'g', 3);
	}
	// values < -1 or > +1:
	for (size_t u = 0; u < UnitCount; ++u)
	{
		if (value > UnitPrefixLargeVal[u])
		{
			return QString::number(value * UnitPrefixSmallVal[u], 'f',
				(value < 10 * UnitPrefixLargeVal[u]) ? 2 : ((value < 100 * UnitPrefixLargeVal[u]) ? 1 : 0)) + UnitPrefixLarge[u];
		}
	}
	// values between 1 and 1000:
	return QString::number(value, 'g', 3);
}

int greatestCommonPrefixLength(QString const & str1, QString const & str2)
{
	int pos = 0;
	while (pos < str1.size() && pos < str2.size() && str1.at(pos) == str2.at(pos))
	{
		++pos;
	}
	return pos;
}

int greatestCommonSuffixLength(QString const & str1, QString const & str2)
{
	int pos = 0;
	while (pos < str1.size() && pos < str2.size()
		   && str1.at(str1.size()-1-pos) == str2.at(str2.size()-1-pos))
	{
		++pos;
	}
	return pos;
}

QString greatestCommonPrefix(QString const & str1, QString const & str2)
{
	return str1.left(greatestCommonPrefixLength(str1, str2));
}

QString greatestCommonSuffix(QString const & str1, QString const & str2)
{
	return str1.right(greatestCommonSuffixLength(str1, str2));
}

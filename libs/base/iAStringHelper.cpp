// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAStringHelper.h"

#include "iAMathUtility.h"

#include <QRegularExpression>


QStringList splitPossiblyQuotedString(QString const & str)
{
	// TODO : rewrite so that it can cope with multiply quoted strings
	//     (where quotes are escaped by \" )
	QStringList result;
	QRegularExpression exp("\\s*([^\"]\\S*|\".*?\")\\s*");
	qsizetype offset = 0;
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
	return result.remove(QRegularExpression("<[^>]*>"));
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
	const QString UnitPrefixLarge[UnitCount] = { "P", "T", "G", "M", "K" };

	// Small Values:
	const double OneMilli = 0.001;
	const double OneMicro = OneMilli * OneMilli;
	const double OneNano  = OneMilli * OneMicro;
	const double OnePico  = OneMilli * OneNano;
	const double OneFemto = OneMilli * OnePico;
	const double UnitPrefixSmallVal[UnitCount] = { OneFemto, OnePico, OneNano, OneMicro, OneMilli };
	const QString UnitPrefixSmall[UnitCount] = { "f", "p", "n", "µ", "m" };
}

QString dblToStringWithUnits(double value, double switchFactor)
{
	if (dblApproxEqual(value, 0.0))
	{
		return "0";
	}
	if (value >= -1.0 && value < 1.0)
	{
		if (std::abs(value) > 0.001 * switchFactor)
		{
			return QString::number(value, 'f', 2);
		}
		switchFactor = clamp(1.0, 1000.0, switchFactor);
		for (size_t u = 0; u < UnitCount; ++u)
		{
			if (value < UnitPrefixSmallVal[u] * switchFactor)
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


// source: https://stackoverflow.com/a/12155571
std::string joinStdString(std::vector<std::string> const & vec, std::string const& joinStr)
{
	return vec.empty() ? "" :           // leave early if there are no items in the list
		std::accumulate(                // otherwise, accumulate
			++vec.begin(), vec.end(),   // the range 2nd to after-last
			*vec.begin(),               // and start accumulating with the first item
			[joinStr](auto const & a, auto const & b) { return a + joinStr + b; });
}

QString joinQVariantAsString(QVector<QVariant> const& vec, QString const& joinStr)
{
	QString result;
	bool first = true;
	for (auto elem : vec)
	{
		if (!first)
		{
			result += joinStr;
		}
		else
		{
			first = false;
		}
		result += elem.toString();
	}
	return result;
}

qsizetype greatestCommonPrefixLength(QString const & str1, QString const & str2)
{
	qsizetype pos = 0;
	while (pos < str1.size() && pos < str2.size() && str1.at(pos) == str2.at(pos))
	{
		++pos;
	}
	return pos;
}

qsizetype greatestCommonSuffixLength(QString const & str1, QString const & str2)
{
	qsizetype pos = 0;
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

int requiredDigits(double value)
{
	return (std::abs(value) <= 1.0) ? 1 :
		static_cast<int>(std::floor(std::log10(std::abs(value))) + 1);
}

int digitsAfterComma(double resolvableDiff)
{
	double diff = std::abs(resolvableDiff);
	if (diff >= 10)
	{
		return 0;
	}
	double logVal = std::log10(1 / diff);
	// ceil instead of floor here (in contrast to requiredDigits above),
	// because corner cases go in other direction:
	// i.e. while 10 already requires 2 digits, 1/10=0.1 still only requires 1 digit
	int digits = static_cast<int>(std::ceil(logVal));
	double tenFactor = std::pow(10, static_cast<int>(digits));
	double multiplied = resolvableDiff * tenFactor;
	// 0.01 as epsilon because they just have to roughly match, we won't show more than 1 additional digit in any case:
	if (!dblApproxEqual(multiplied, std::round(multiplied), 0.01 ))
	{	// for differences like 0.15 (in contrast to a round 0.1) we want to add an extra digit:
		++digits;
	}
	return digits;
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include "iALog.h"

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include <cassert>

class QString;

//! Class for converting a variable of the type QString to the templated type.
//! Can be overloaded for conversions to and from custom types.
template <typename T>
struct iAConverter
{
	static T toT(QString /*string*/, bool * ok)
	{
		assert(false && "Unspecialized Converter::toT called! This should not happen!");
		if (ok)
		{
			*ok = false;
		}
		return std::numeric_limits<T>::signaling_NaN();
	}
	static QString toString(T /*number*/)
	{
		assert(false && "Unspecialized Converter::toString called! This should not happen!");
		return "";
	}
};

//! Converts (signed) char to and from QString.
template <>
struct iAConverter<char>
{
	static char toT(QString str, bool* ok)
	{
		return static_cast<char>(str.toInt(ok));
	}
	static QString toString(char n)
	{
		return QString::number(n);
	}
};

//! Converts unsigned char to and from QString.
template <>
struct iAConverter<unsigned char>
{
	static unsigned char toT(QString str, bool* ok)
	{
		return static_cast<unsigned char>(str.toInt(ok));
	}
	static QString toString(unsigned char n)
	{
		return QString::number(n);
	}
};

//! Converts (signed) short to and from QString.
template <>
struct iAConverter<short>
{
	static short toT(QString str, bool* ok)
	{
		return str.toShort(ok);
	}
	static QString toString(short n)
	{
		return QString::number(n);
	}
};

//! Converts unsigned short to and from QString.
template <>
struct iAConverter<unsigned short>
{
	static unsigned short toT(QString str, bool* ok)
	{
		return str.toUShort(ok);
	}
	static QString toString(unsigned short n)
	{
		return QString::number(n);
	}
};

//! Converts (signed) int to and from QString.
template <>
struct iAConverter<int>
{
	static int toT(QString str, bool * ok)
	{
		return str.toInt(ok);
	}
	static QString toString(int n)
	{
		return QString::number(n);
	}
};

//! Converts unsigned int to and from QString.
template <>
struct iAConverter<unsigned int>
{
	static unsigned int toT(QString str, bool* ok)
	{
		return str.toUInt(ok);
	}
	static QString toString(unsigned int n)
	{
		return QString::number(n);
	}
};

//! Converts size type (typically 64 bit unsigned) to and from QString.
template <>
struct iAConverter<size_t>
{
	static size_t toT(QString str, bool* ok)
	{
		return str.toULongLong(ok);
	}
	static QString toString(size_t n)
	{
		return QString::number(n);
	}
};

//! Converts a 64 bit integer to and from QString.
template <>
struct iAConverter<int64_t>
{
	static int64_t toT(QString str, bool* ok)
	{
		return str.toLongLong(ok);
	}
	static QString toString(int64_t n)
	{
		return QString::number(n);
	}
};

//! Converts double to and from QString.
template <>
struct iAConverter<double>
{
	static double toT(QString str, bool * ok)
	{
		return str.toDouble(ok);
	}
	static QString toString(double n)
	{
		return QString::number(n);
	}
};

//! Converts float to and from QString.
template <>
struct iAConverter<float>
{
	static float toT(QString str, bool* ok)
	{
		return str.toFloat(ok);
	}
	static QString toString(float n)
	{
		return QString::number(n);
	}
};

//! Converts bool to and from QString.
template <>
struct iAConverter<bool>
{
	static bool toT(QString str, bool * ok)
	{
		auto ls = str.toLower();
		if (ok)
		{
			*ok =
				ls == "on" || ls == "off" ||
				ls == "true" || ls == "false";
		}
		return ls == "on" || ls == "true";
	}
	static QString toString(bool b)
	{
		return b ? "on" : "off";
	}
};



//! split a string at the space characters, while correctly treating quoted elements
//!
//! Example: the string '"a rabbit" and "a horse"' would be split into three elements:
//!     "a rabbit", "and", "a horse" (the quotes are stripped from the elements).
//!     Note that only the double-quote character is considered as a quote by this function.
//! @param str the string to split
//! @return a list of strings split up at the whitespaces
iAbase_API QStringList splitPossiblyQuotedString(QString const & str);

iAbase_API QString quoteString(QString const & str);

//! Convert a given string representation to an array of given type with given number of elements
template <typename ValT, typename ContainerT>
bool stringToArray(QString const & str, ContainerT arr, int expectedSize, QString const & sep = " ")
{
	QStringList list = str.split(sep);
	for (QStringList::size_type i = 0; i < expectedSize && i < list.size(); ++i)
	{
		bool ok;
		arr[i] = iAConverter<ValT>::toT(list[i], &ok);
		if (!ok)
		{
			return false;
		}
	}
	return (list.size() == expectedSize);
}

//! Convert a given array with specified number of elements to a string representation
template <typename T>
QString arrayToString(T const * arr, size_t size, QString const & sep = " ")
{
	QString result;
	for (size_t i = 0; i < size; ++i)
	{
		result += iAConverter<T>::toString(arr[i]);
		if (i < size - 1)
		{
			result += sep;
		}
	}
	return result;
}

template <typename T, size_t N>
bool stringToArray(QString const& str, std::array<T, N>& ar, QString const& sep = ", ")
{
	return stringToArray<T>(str, ar.data(), N, sep);
}

template <typename T, size_t N>
QString arrayToString(std::array<T, N> const & ar, QString const& sep = ", ")
{
	return arrayToString(ar.data(), N, sep);
}

//! Split a string with multiple values, and put values into a container.
//! @param val a container (required to have an operator[] to set elements).
//! @param str the string to be split.
//! @param sep the separator between values.
template <typename T, typename TVal, int Nval>
void valuesFromString(T& val, QString const & str, QString const & sep = " ")
{
	QStringList list = str.split(sep, Qt::SkipEmptyParts);
	if (list.size() >= Nval)
	{
		for (int j = 0; j < Nval; j++)
		{	// TODO: check why we don't use iAConverter<TVal>::toT here!
			val[j] = static_cast<TVal>(list.at(j).toDouble());
		}
	}
	// else report error?
}

template <typename ContainerT, typename ElementT>
ContainerT stringToVector(QString const& listAsString, QString const& separator=",", qsizetype maxItems=std::numeric_limits<qsizetype>::max(),
	bool warnWhenInvalid = true)
{
	QStringList strList = listAsString.split(separator);
	ContainerT result(std::min(strList.size(), maxItems));
	for (qsizetype i = 0; i < strList.size() && i < maxItems; ++i)
	{
		bool ok;
		result[i] = iAConverter<ElementT>::toT(strList[i], &ok);
		if (!ok && warnWhenInvalid)
		{
			LOG(lvlWarn, QString("Invalid value %1 in stringToVector conversion!").arg(strList[i]));
		}
	}
	return result;
}

//! Pads or truncates the given string to the given size.
//!
//! If the string given in name is longer than the specified size, the string is truncated
//! to size-2 and ".." is appended, otherwise it is filled with spaces to be exactly size long
//! @param str the string to be padded or truncated
//! @param size the size that the return string should have
//! @return a string of exactly the given size, padded or truncated from the given name
iAbase_API QString padOrTruncate(QString const & str, int size);

//! strip HTML tags from the given string
//! @param html a string potentially containing HTML tags
//! @return the input string with all HTML tags (\<xyz\>, \</xyz\>, \<xyz/\>) removed
iAbase_API QString stripHTML(QString const & html);

//! returns the value converted to string, with units (K, M, G, T, P) applied for every 10³ factor over 1000
//! @param value the value to convert to a string
//! @param switchFactor the multiple of the "base unit" under which to switch to next lower unit;
//!      example: if value = 0.101 and switchFactor is at default 100, the returned string will be "0.1"
//!               with the same value and a switchFactor of 102 (or any other value larger than 101),
//!               the result will be 101m
iAbase_API QString dblToStringWithUnits(double value, double switchFactor = 999);

//! join a vector of std::strings into one single string
iAbase_API std::string joinStdString(std::vector<std::string> const & vec, std::string const& joinStr = ",");

//! join any list as string - the conversion of the single items happens via the passed-in lambda
//! FnType is something like a function taking an Element parameter and has a QString(-compatible)
//! return type; tried QString fct(Element const &) but it doesn't work as expected
template <template <typename...> class Container, typename Element, typename FnType>
QString joinAsString(Container<Element> const& vec, QString const& joinStr, FnType lambda)
{
	QString result;
	bool first = true;
	for (Element elem : vec)
	{
		if (!first)
		{
			result += joinStr;
		}
		else
		{
			first = false;
		}
		result += lambda(elem);
	}
	return result;
}

//! join an iterable collection of numeric elements to a string
//!
//! works similar to QString::join, but on arbitrary iterable collection types
//! containing items which can be converted to QString via QString::number.
//! @param vec the collection of elements to be joined
//! @param joinStr the string to be used in between the elements of the string
//! @return a string joining all elements of the given collection together
template <template <typename...> class Container, typename Element>
QString joinNumbersAsString(Container<Element> const& vec, QString const& joinStr)
{
	return joinAsString(vec, joinStr, [](Element const& elem) -> QString { return QString::number(elem); });
}

iAbase_API QString joinQVariantAsString(QVector<QVariant> const& vec, QString const& joinStr);

//! Find the (length of the) greatest common prefix of the two given strings.
//! example: str1 ="BaseMethod", str2="BaseMember" => result: "BaseMe"
iAbase_API qsizetype greatestCommonPrefixLength(QString const & str1, QString const & str2);
iAbase_API QString greatestCommonPrefix(QString const & str1, QString const & str2);

iAbase_API qsizetype greatestCommonSuffixLength(QString const & str1, QString const & str2);
iAbase_API QString greatestCommonSuffix(QString const & str1, QString const & str2);

//! Get the number of digits required for the given number (before the comma).
iAbase_API int requiredDigits(double value);

//! Get the number of digits required after the comma,
//! given the difference to other values it should be distinguishable from.
//! TODO: introduce "number of relevant digits" parameter / automatic determination?
//!      e.g. 9.125 -> 3, 9.98 -> 2, 9.5 -> 1; but what about e.g. 9.995 (->close enough to 10 to discard after comma?)
//! Examples:
//! resolvableDiff  result
//! >= 10           0
//! 9.9999 - 1.0    0-1 (0 if resolvableDiff is exactly 1,6,4,... depending on whether resolvableDiff is 1.2,6.323, or )
//! 0.9999 - 0.1    1-2 (depending on whether resolvableDiff is 0.15,0.22,0.9333,... or exactly 0.1,0.4,...)
//! 0.0999 - 0.01   2-3
//! 0.0099 - 0.001  3-4
iAbase_API int digitsAfterComma(double resolvableDiff);

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVariant>
//#include <QVector>

#include "iAStringHelper.h"

//! This file contains helpers for dealing with "vector" types (iAValueType::Vector*);
//! Internally, these are stored as strings.
//! This is so that the stored values in e.g. project files still are nicely readable
//! (and use less storage than the elaborate results that serializing a QVector yields)

namespace
{
	constexpr const char Separator[] = ",";
	constexpr const char FallbackSeparator[] = " ";
}

//! Set any indexed type from a iAValueType::Vector3 stored in a QVariant (as in the QVariantMap used throughout open_iA)
// TODO: check for number of values!
template <typename T, typename U>
bool setFromVectorVariant(U& dest, QVariant const& src)
{
	auto values = src.toString().split(Separator);
	bool result = true;
	for (int i = 0; i < values.size(); ++i)
	{
		bool ok;
		dest[i] = iAConverter<T>::toT(values[i], &ok);
		if (!ok)
		{
			result = false;
			LOG(lvlDebug, QString("setFromVectorVariant: Could not convert element %1 (%2); full variant: %3").arg(i).arg(values[i]).arg(src.toString()));
		}
	}
	return result;
}

template <typename T>
QVector<T> variantToVector(QVariant const& src)
{
	auto values = src.toString().split(Separator);
	if (values.size() == 1)  // fallback to try " " as separator (used in older versions)
	{
		values = src.toString().split(FallbackSeparator);
	}
	QVector<T> result(values.size());
	for (int i = 0; i < values.size(); ++i)
	{
		bool ok;
		result[i] = iAConverter<T>::toT(values[i], &ok);
		if (!ok)
		{
			LOG(lvlDebug, QString("variantToVector: Could not convert element %1 (%2); full variant: %3").arg(i).arg(values[i]).arg(src.toString()));
		}
	}
	return result;
}

//! Create a vector QVariant (first templated type) from an initializer list
template <typename T>
QVariant variantVector(std::initializer_list<T> s)
{
	//return QVariant::fromValue(QVector<T>(s));
	QString str;
	for (auto e : s)
	{
		if (!str.isEmpty())
		{
			str +=  Separator;
		}
		str += iAConverter<T>::toString(e);
	}
	return QVariant(str);
}

//! Create a vector QVariant from an QVector of a numeric type
template <typename T>
QVariant variantVector(QVector<T> s)
{
	QString str;
	for (auto e : s)
	{
		if (!str.isEmpty())
		{
			str += Separator;
		}
		str += iAConverter<T>::toString(e);
	}
	return QVariant(str);
}

//! Create a QVariant with a QVector<T> (first templated type) from an array d with n elements
template <typename T>
QVariant variantVector(T const* d, int n)
{
	QString str;
	for (int i=0; i<n; ++i)
	{
		if (!str.isEmpty())
		{
			str += Separator;
		}
		str += iAConverter<T>::toString(d[i]);
	}
	return QVariant(str);
}

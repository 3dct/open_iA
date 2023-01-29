// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QVariant>
#include <QVector>

//using iAVecType  = QVector<double>;
//using iAVecIType = QVector<int>;

//! Set any indexed type from a QVector stored in a QVariant (as in the QVariantMap used throughout open_iA)
template <typename T, typename U>
void setFromVectorVariant(U& dest, QVariant const& src)
{
	auto vec = src.value<QVector<T>>();
	for (int i = 0; i < vec.size(); ++i)
	{
		dest[i] = vec[i];
	}
}

//! Create a QVariant with a QVector<T> (first templated type) from an initializer list
template <typename T>
QVariant variantVector(std::initializer_list<T> s)
{
	return QVariant::fromValue(QVector<T>(s));
}

template <typename T>
QVariant variantVector(T const* d, int n)
{
	QVector<T> vec(n);
	for (int i = 0; i < n; ++i)
	{
		vec[i] = d[i];
	}
	return QVariant::fromValue(vec);
}

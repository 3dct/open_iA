/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

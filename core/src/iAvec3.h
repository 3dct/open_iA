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

#include <algorithm>  // for std::fill (in Linux)
#include <cmath>  // for std::acos

//!	Class representing 3 dimensional vector.
template <typename RealType>
class iAVec3T
{
public:
	iAVec3T();
	template <typename T1, typename T2, typename T3> explicit iAVec3T(T1 px, T2 py, T3 pz);
	template <typename ParamType> explicit iAVec3T(ParamType val);
	template <typename ParamType> explicit iAVec3T(ParamType data[3]);
	template <typename ParamType> iAVec3T(const iAVec3T<ParamType>& v);

	RealType x() const { return values[0]; }
	RealType y() const { return values[1]; }
	RealType z() const { return values[2]; }

	template <typename ParamType> iAVec3T<RealType>& operator= (const iAVec3T<ParamType>& v);
	template <typename ParamType> iAVec3T<RealType>& operator= (ParamType d);
	iAVec3T<RealType> operator+ () const;
	iAVec3T<RealType> operator- () const;
	template <typename ParamType> iAVec3T<RealType>& operator+= (const iAVec3T<ParamType>& v);
	template <typename ParamType> iAVec3T<RealType>& operator-= (const iAVec3T<ParamType>& v);
	template <typename ParamType> iAVec3T<RealType>& operator*= (const iAVec3T<ParamType>& v);
	template <typename ParamType> iAVec3T<RealType>& operator*= (ParamType f);
	template <typename ParamType> iAVec3T<RealType>& operator/= (const iAVec3T<ParamType>& v);
	const RealType& operator[] (size_t index) const;
	RealType& operator[] (size_t index);
	RealType length() const;
	RealType sum() const;
	void normalize();
	iAVec3T<RealType> normalized() const;
	RealType * data();
private:
	RealType values[3];
};

typedef iAVec3T<float> iAVec3;
//typedef iAVec3T<double> iAVec3d;


template <typename RealType>
iAVec3T<RealType>::iAVec3T()
{
	std::fill(values, values + 3, static_cast<RealType>(0.0));
}

template <typename RealType>
template <typename T1, typename T2, typename T3>
iAVec3T<RealType>::iAVec3T(T1 px, T2 py, T3 pz)
{
	values[0] = static_cast<RealType>(px);
	values[1] = static_cast<RealType>(py);
	values[2] = static_cast<RealType>(pz);
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>::iAVec3T(ParamType val)
{
	std::fill(values, values + 3, static_cast<RealType>(val) );
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>::iAVec3T(ParamType data[3])
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<RealType>(data[i]);
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>::iAVec3T(const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<RealType>(v.values[i]);
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<RealType>(v.values[i]);
	return *this;
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator= (ParamType d)
{
	for (int i = 0; i < 3; ++i)
		values[i] = static_cast<RealType>(d);
	return *this;
}

template <typename RealType>
RealType iAVec3T<RealType>::length() const
{
	return std::sqrt(x()*x() + y() * y() + z() * z());
}

template <typename RealType>
RealType iAVec3T<RealType>::sum() const
{
	return x() + y() + z();
}

template <typename RealType>
iAVec3T<RealType> iAVec3T<RealType>::operator+ () const
{
	return *this;
}

template <typename RealType>
iAVec3T<RealType> iAVec3T<RealType>::operator- () const
{
	return iAVec3T<RealType>(-x(), -y(), -z());
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator+= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] += static_cast<RealType>(v.values[i]);
	return *this;
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator-= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] -= static_cast<RealType>(v.values[i]);
	return *this;
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator*= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] *= static_cast<RealType>(v.values[i]);
	return *this;
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator*= (ParamType f)
{
	for (int i = 0; i<3; ++i)
		values[i] *= static_cast<RealType>(f);
	return *this;
}

template <typename RealType>
template <typename ParamType>
iAVec3T<RealType>& iAVec3T<RealType>::operator/= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] /= static_cast<RealType>(v.values[i]);
	return *this;
}

template <typename RealType>
const RealType& iAVec3T<RealType>::operator[] (size_t index) const
{
	return values[index];
}

template <typename RealType>
RealType& iAVec3T<RealType>::operator[] (size_t index)
{
	return values[index];
}

template <typename RealType>
void iAVec3T<RealType>::normalize()
{
	*this = this->normalized();
}

template <typename RealType>
iAVec3T<RealType> iAVec3T<RealType>::normalized() const
{
	RealType l = this->length();
	return (l > 0) ? (*this) / l : (*this);
}

template <typename RealType>
RealType * iAVec3T<RealType>::data()
{
	return values;
}

// BINARY FUNCTIONS/OPERATORS:

template <typename RealType>
bool operator== (const iAVec3T<RealType>& u, const iAVec3T<RealType>& v)
{
	return u.x() == v.x() && u.y() == v.y() && u.z() == v.z();
}

template <typename RealType>
bool operator!= (const iAVec3T<RealType>& u, const iAVec3T<RealType>& v)
{
	return u.x() != v.x() || u.y() != v.y() || u.z() != v.z();
}

template <typename RealType>
bool operator< (const iAVec3T<RealType>& u, const iAVec3T<RealType>& v)
{
	return (u.x() < v.x()) || (u.x() == v.x() && u.y() < v.y()) || (u.x() == v.x() && u.y() == v.y() && u.z() < v.z());
}

template <typename RealType>
bool operator> (const iAVec3T<RealType>& u, const iAVec3T<RealType>& v)
{
	return (u.x() > v.x()) || (u.x() == v.x() && u.y() > v.y()) || (u.x() == v.x() && u.y() == v.y() && u.z() > v.z());
}

template <typename T1, typename T2>
iAVec3T<T1> operator + (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() + static_cast<T1>(v.x()), u.y() + static_cast<T1>(v.y()), u.z() + static_cast<T1>(v.z()));
}

template <typename T1>
iAVec3T<T1> operator + (const iAVec3T<T1>& u, T1 d)
{
	return iAVec3T<T1>(u.x() + d, u.y() + d, u.z() + d);
}

template <typename T1, typename T2>
iAVec3T<T1> operator - (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() - static_cast<T1>(v.x()), u.y() - static_cast<T1>(v.y()), u.z() - static_cast<T1>(v.z()));
}

template <typename T1>
iAVec3T<T1> operator - (const iAVec3T<T1>& u, T1 d)
{
	return iAVec3T<T1>(u.x() - d, u.y() - d, u.z() - d);
}

template <typename T1, typename T2>
iAVec3T<T1> operator * (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() * static_cast<T1>(v.x()), u.y() * static_cast<T1>(v.y()), u.z() * static_cast<T1>(v.z()));
}

template <typename RealType, typename ParamType>
iAVec3T<RealType> operator * (const iAVec3T<RealType>& v, ParamType a)
{
	RealType b = static_cast<RealType>(a);
	return iAVec3T<RealType>(v.x() * b, v.y() * b, v.z() * b);
}

template <typename RealType, typename ParamType>
iAVec3T<RealType> operator * (ParamType a, const iAVec3T<RealType>& v)
{
	return v * a;
}

template <typename RealType, typename ParamType>
iAVec3T<RealType> operator / (const iAVec3T<RealType>& v, ParamType a)
{
	RealType b = static_cast<RealType>(a);
	return iAVec3T<RealType>(v.x() / b, v.y() / b, v.z() / b);
}

template <typename T1, typename T2>
iAVec3T<T1> operator / (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() / static_cast<T1>(v.x()), u.y() / static_cast<T1>(v.y()), u.z() / static_cast<T1>(v.z()));
}

template <typename T1, typename T2>
T1 dotProduct(const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return u.x()*static_cast<T1>(v.x()) + u.y()*static_cast<T1>(v.y()) + u.z()*static_cast<T1>(v.z());
}

template <typename T1, typename T2>
T1 operator & (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return dotProduct(u, v);
}

template <typename T1, typename T2>
iAVec3T<T1> crossProduct(const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(
		u.y()*static_cast<T1>(v.z()) - u.z()*static_cast<T1>(v.y()),
		u.z()*static_cast<T1>(v.x()) - u.x()*static_cast<T1>(v.z()),
		u.x()*static_cast<T1>(v.y()) - u.y()*static_cast<T1>(v.x()));
}

template <typename T1, typename T2>
iAVec3T<T1> operator ^ (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return crossProduct(u, v);
}

template <typename T1, typename T2>
T1 angle(iAVec3T<T1> const & u, iAVec3T<T2> const & v)
{
	return (u.length() == 0 || v.length() == 0) ? 0 :
		std::acos(dotProduct(u, v) / (u.length() * static_cast<T1>(v.length())));
}

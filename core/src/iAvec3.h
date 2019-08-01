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
#pragma once

#include <algorithm>  // for std::fill (in Linux)
#include <cmath>      // for std::acos

//!	Class representing a 3-dimensional vector.
template <typename T>
class iAVec3T
{
public:
	//! initialize empty vector (all components =0)
	iAVec3T();
	//! initialize all vector components to the same value
	explicit iAVec3T(T val);
	//! initialize vector from three values (of potentially different type)
	explicit iAVec3T(T px, T py, T pz);
	//! initialize vector components from an array
	explicit iAVec3T(T data[3]);
	//! initialize vector components from another vector (of potentially different type)
	iAVec3T(const iAVec3T<T>& v);

	//! assign another vector
	template <typename ParamType> iAVec3T<T>& operator= (const iAVec3T<ParamType>& v);
	//! assign a single value to all components
	template <typename ParamType> iAVec3T<T>& operator= (ParamType d);

	//! access the x component of the vector
	T x() const { return values[0]; }
	//! access the y component of the vector
	T y() const { return values[1]; }
	//! access the z component of the vector
	T z() const { return values[2]; }

	//! constant indexed access to the components of this vector; index=0 -> x, index=1 -> y index=2 -> z
	const T& operator[] (size_t index) const;
	//! indexed read&write access to the components of this vector; index=0 -> x, index=1 -> y index=2 -> z
	T& operator[] (size_t index);
	//! access to the raw array of size 3 holding the components
	T * data();
	//! access to the raw array of size 3 holding the components (const version)
	T const * data() const;

	//! get the length of the vector, alias for magnitude()
	T length() const;
	//! get the length of the vector
	T magnitude() const;
	//! get the squared length of the vector
	T sqrMagnitude() const;
	//! get a normalize vector, i.e., a vector of length=1 pointing in the same direction
	iAVec3T<T> normalized() const;
	//! get the sum of all vector components
	T sum() const;
	//! make this vector normalized, i.e. set the length to 1 but keep direction
	void normalize();

	//! unary + operator. Note: this is not handling the addition of two vectors! It just allows to write " +someVectorVariable" as an expression, it returns the vector itself
	iAVec3T<T> operator+ () const;
	//! unary - operator. Writing "-someVectorVariable" returns a vector with all components of someVectorVariable multiplied by -1
	iAVec3T<T> operator- () const;
	//! add another vector to this
	template <typename ParamType> iAVec3T<T>& operator+= (const iAVec3T<ParamType>& v);
	//! subtract another vector from this
	template <typename ParamType> iAVec3T<T>& operator-= (const iAVec3T<ParamType>& v);
	//! multiply another vector with this
	template <typename ParamType> iAVec3T<T>& operator*= (const iAVec3T<ParamType>& v);
	//! multiply a constant to every component of this
	template <typename ParamType> iAVec3T<T>& operator*= (ParamType f);
	//! divide all components of this vector by the respective components of another vector
	template <typename ParamType> iAVec3T<T>& operator/= (const iAVec3T<ParamType>& v);

private:
	//! array holding the vector components
	T values[3];
};

// Predefined types
typedef iAVec3T<float>   iAVec3f;
typedef iAVec3T<double>  iAVec3d;
typedef iAVec3T<int>     iAVec3i;

//! @{
//! Comparison operators for two vectors. Note that only two vectors of the same type can be compared
template <typename T> bool operator== (const iAVec3T<T>& u, const iAVec3T<T>& v);
template <typename T> bool operator!= (const iAVec3T<T>& u, const iAVec3T<T>& v);
template <typename T> bool operator<  (const iAVec3T<T>& u, const iAVec3T<T>& v);
template <typename T> bool operator>  (const iAVec3T<T>& u, const iAVec3T<T>& v);
//! @

//! @{
//! basic arithmetical operations on two vectors / a vector and a value
template <typename T1, typename T2> iAVec3T<T1> operator + (const iAVec3T<T1>& u, const iAVec3T<T2>& v);
template <typename T1, typename T2> iAVec3T<T1> operator + (const iAVec3T<T1>& u, T2 a);
template <typename T1, typename T2> iAVec3T<T2> operator + (T1 a, const iAVec3T<T2>& u);
template <typename T1, typename T2> iAVec3T<T1> operator - (const iAVec3T<T1>& u, const iAVec3T<T2>& v);
template <typename T1, typename T2> iAVec3T<T1> operator - (const iAVec3T<T1>& u, T2 a);
template <typename T1, typename T2> iAVec3T<T1> operator * (const iAVec3T<T1>& u, const iAVec3T<T2>& v);
template <typename T1, typename T2> iAVec3T<T1> operator * (const iAVec3T<T1>& u, T2 a);
template <typename T1, typename T2> iAVec3T<T2> operator * (T1 a, const iAVec3T<T2>& u);
template <typename T1, typename T2> iAVec3T<T1> operator / (const iAVec3T<T1>& u, T2 a);
template <typename T1, typename T2> iAVec3T<T1> operator / (const iAVec3T<T1>& u, const iAVec3T<T2>& v);
//! @}

//! alias for dot product (deprecated!)
template <typename T1, typename T2> T1 operator & (const iAVec3T<T1>& u, const iAVec3T<T2>& v);
//! alias for cross product (deprecated!)
template <typename T1, typename T2> iAVec3T<T1> operator ^ (const iAVec3T<T1>& u, const iAVec3T<T2>& v);


//! get dot product between two vectors
template <typename T1, typename T2> static T1 dotProduct(const iAVec3T<T1>& u, const iAVec3T<T2>& v);
//! get cross product between two vectors
template <typename T1, typename T2> static iAVec3T<T1> crossProduct(const iAVec3T<T1>& u, const iAVec3T<T2>& v);
//! get angle between two vectors (in radians)
template <typename T1, typename T2> static T1 angleBetween(iAVec3T<T1> const & u, iAVec3T<T2> const & v);


template <typename T>
iAVec3T<T>::iAVec3T()
{
	std::fill(values, values + 3, static_cast<T>(0.0));
}

template <typename T>
iAVec3T<T>::iAVec3T(T px, T py, T pz)
{
	values[0] = static_cast<T>(px);
	values[1] = static_cast<T>(py);
	values[2] = static_cast<T>(pz);
}

template <typename T>
iAVec3T<T>::iAVec3T(T val)
{
	std::fill(values, values + 3, static_cast<T>(val) );
}

template <typename T>
iAVec3T<T>::iAVec3T(T data[3])
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<T>(data[i]);
}

template <typename T>
iAVec3T<T>::iAVec3T(const iAVec3T<T>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<T>(v.values[i]);
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] = static_cast<T>(v.values[i]);
	return *this;
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator= (ParamType d)
{
	for (int i = 0; i < 3; ++i)
		values[i] = static_cast<T>(d);
	return *this;
}

template <typename T>
T iAVec3T<T>::length() const
{
	return magnitude();
}

template<typename T>
T iAVec3T<T>::magnitude() const
{
	return std::sqrt(sqrMagnitude());
}

template<typename T>
T iAVec3T<T>::sqrMagnitude() const
{
	return x()*x() + y()*y() + z()*z();
}

template <typename T>
T iAVec3T<T>::sum() const
{
	return x() + y() + z();
}

template <typename T>
iAVec3T<T> iAVec3T<T>::operator+ () const
{
	return *this;
}

template <typename T>
iAVec3T<T> iAVec3T<T>::operator- () const
{
	return iAVec3T<T>(-x(), -y(), -z());
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator+= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] += static_cast<T>(v.values[i]);
	return *this;
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator-= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] -= static_cast<T>(v.values[i]);
	return *this;
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator*= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] *= static_cast<T>(v.values[i]);
	return *this;
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator*= (ParamType f)
{
	for (int i = 0; i<3; ++i)
		values[i] *= static_cast<T>(f);
	return *this;
}

template <typename T>
template <typename ParamType>
iAVec3T<T>& iAVec3T<T>::operator/= (const iAVec3T<ParamType>& v)
{
	for (int i = 0; i<3; ++i)
		values[i] /= static_cast<T>(v.values[i]);
	return *this;
}

template <typename T>
const T& iAVec3T<T>::operator[] (size_t index) const
{
	return values[index];
}

template <typename T>
T& iAVec3T<T>::operator[] (size_t index)
{
	return values[index];
}

template <typename T>
void iAVec3T<T>::normalize()
{
	*this = this->normalized();
}

template <typename T>
iAVec3T<T> iAVec3T<T>::normalized() const
{
	T l = this->length();
	return (l > 0) ? (*this) / l : (*this);
}

template <typename T>
T * iAVec3T<T>::data()
{
	return values;
}

template <typename T>
T const * iAVec3T<T>::data() const
{
	return values;
}

// BINARY FUNCTIONS/OPERATORS:

template <typename T>
bool operator== (const iAVec3T<T>& u, const iAVec3T<T>& v)
{
	return u.x() == v.x() && u.y() == v.y() && u.z() == v.z();
}

template <typename T>
bool operator!= (const iAVec3T<T>& u, const iAVec3T<T>& v)
{
	return u.x() != v.x() || u.y() != v.y() || u.z() != v.z();
}

template <typename T>
bool operator< (const iAVec3T<T>& u, const iAVec3T<T>& v)
{
	return (u.x() < v.x()) || (u.x() == v.x() && u.y() < v.y()) || (u.x() == v.x() && u.y() == v.y() && u.z() < v.z());
}

template <typename T>
bool operator> (const iAVec3T<T>& u, const iAVec3T<T>& v)
{
	return (u.x() > v.x()) || (u.x() == v.x() && u.y() > v.y()) || (u.x() == v.x() && u.y() == v.y() && u.z() > v.z());
}

template <typename T1, typename T2>
iAVec3T<T1> operator + (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() + static_cast<T1>(v.x()), u.y() + static_cast<T1>(v.y()), u.z() + static_cast<T1>(v.z()));
}

template <typename T1, typename T2>
iAVec3T<T1> operator + (const iAVec3T<T1>& u, T2 a)
{
	return iAVec3T<T1>(u.x() + static_cast<T1>(a), u.y() + static_cast<T1>(a), u.z() + static_cast<T1>(a));
}

template <typename T1, typename T2>
iAVec3T<T2> operator + (T1 a, const iAVec3T<T2>& u)
{
	return u + a;
}

template <typename T1, typename T2>
iAVec3T<T1> operator - (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() - static_cast<T1>(v.x()), u.y() - static_cast<T1>(v.y()), u.z() - static_cast<T1>(v.z()));
}

template <typename T1, typename T2>
iAVec3T<T1> operator - (const iAVec3T<T1>& u, T2 a)
{
	return iAVec3T<T1>(u.x() - static_cast<T1>(a), u.y() - static_cast<T1>(a), u.z() - static_cast<T1>(a));
}

template <typename T1, typename T2>
iAVec3T<T1> operator * (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(u.x() * static_cast<T1>(v.x()), u.y() * static_cast<T1>(v.y()), u.z() * static_cast<T1>(v.z()));
}

template <typename T1, typename T2>
iAVec3T<T1> operator * (const iAVec3T<T1>& v, T2 a)
{
	T1 b = static_cast<T1>(a);
	return iAVec3T<T1>(v.x() * b, v.y() * b, v.z() * b);
}

template <typename T1, typename T2>
iAVec3T<T2> operator * (T1 a, const iAVec3T<T2>& u)
{
	return u * a;
}

template <typename RealType, typename ParamType>
iAVec3T<RealType> operator / (const iAVec3T<RealType>& u, ParamType a)
{
	RealType b = static_cast<RealType>(a);
	return iAVec3T<RealType>(u.x() / b, u.y() / b, u.z() / b);
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
iAVec3T<T1> crossProduct(const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return iAVec3T<T1>(
		u.y()*static_cast<T1>(v.z()) - u.z()*static_cast<T1>(v.y()),
		u.z()*static_cast<T1>(v.x()) - u.x()*static_cast<T1>(v.z()),
		u.x()*static_cast<T1>(v.y()) - u.y()*static_cast<T1>(v.x()));
}

template <typename T1, typename T2>
T1 angleBetween(iAVec3T<T1> const & u, iAVec3T<T2> const & v)
{
	return (u.length() == 0 || v.length() == 0) ? 0 :
		std::acos(dotProduct(u, v) / (u.length() * static_cast<T1>(v.length())));
}

// deprecated aliases for dot/cross product:

template <typename T1, typename T2>
T1 operator & (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return dotProduct(u, v);
}

template <typename T1, typename T2>
iAVec3T<T1> operator ^ (const iAVec3T<T1>& u, const iAVec3T<T2>& v)
{
	return crossProduct(u, v);
}

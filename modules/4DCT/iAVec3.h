/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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

#include <cstdlib>
#include <cmath>

// forward declaration
template<typename T> class Vec3;

// shortcuts
typedef Vec3<float>		Vec3f;
typedef Vec3<double>	Vec3d;
typedef Vec3<int>		Vec3i;

template<typename T>
class Vec3
{
public:
	Vec3( );
	Vec3( T x, T y, T z );
	Vec3( const T * v );
	T			magnitude( ) const;
	T			sqrMagnitude( ) const;
	Vec3<T>		normalized( );
	static T	angle( Vec3<T> const & a, Vec3<T> const & b );
	Vec3<T> &	operator+=( const Vec3 & other );
	Vec3<T> &	operator-=( const Vec3 & other );
	template<typename ParamType>
	Vec3<T> &	operator*=( ParamType val );
	template<typename ParamType>
	Vec3<T> &	operator/=( ParamType val );
	T &			operator[]( std::size_t ind );
	const T&	operator[]( std::size_t ind ) const;

	template<typename Tf> friend Vec3<Tf> operator+( const Vec3<Tf> & lhs, const Vec3<Tf> & rhs );
	template<typename Tf> friend Vec3<Tf> operator-( const Vec3<Tf> & lhs, const Vec3<Tf> & rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator+( ParamType lhs, const Vec3<Tf> & rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator+( const Vec3<Tf> & lhs, ParamType rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator*( const Vec3<Tf> & lhs, ParamType rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator*( ParamType lhs, const Vec3<Tf> & rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator/( const Vec3<Tf> & lhs, const ParamType & rhs );
	template<typename Tf, typename ParamType> friend Vec3<Tf> operator/( const ParamType & lhs, const Vec3<Tf> & rhs );
	template<typename Tf> friend Tf operator&( const Vec3<Tf> & lhs, const Vec3<Tf> & rhs ); // dot production
private:
	T m_val[3];
};

template<typename T>
inline Vec3<T>::Vec3( )
{
	m_val[0] = m_val[1] = m_val[2] = 0;
}

template<typename T>
inline Vec3<T>::Vec3( T x, T y, T z )
{
	m_val[0] = x; m_val[1] = y; m_val[2] = z;
}

template<typename T>
inline Vec3<T>::Vec3( const T * v )
{
	m_val[0] = v[0]; m_val[1] = v[1]; m_val[2] = v[2];
}

template<typename T>
inline T Vec3<T>::magnitude( ) const
{
	return sqrt( sqrMagnitude( ) );
}

template<typename T>
inline T Vec3<T>::sqrMagnitude( ) const
{
	return m_val[0] * m_val[0] + m_val[1] * m_val[1] + m_val[2] * m_val[2];
}

template<typename T>
inline Vec3<T> Vec3<T>::normalized( )
{
	T l = magnitude( );
	return Vec3<T>( m_val[0] / l, m_val[1] / l, m_val[2] / l );
}

template<typename T>
inline T Vec3<T>::angle( Vec3<T> const & a, Vec3<T> const & b )
{
	return std::acos( ( a & b ) / ( a.magnitude( ) * b.magnitude( ) ) );
}

template<typename T>
inline Vec3<T>& Vec3<T>::operator+=( const Vec3 & other )
{
	m_val[0] += other.m_val[0]; m_val[1] += other.m_val[1]; m_val[2] += other.m_val[2];
	return *this;
}

template<typename T>
inline Vec3<T>& Vec3<T>::operator-=( const Vec3<T> & other )
{
	m_val[0] -= other.m_val[0]; m_val[1] -= other.m_val[1]; m_val[2] -= other.m_val[2];
	return *this;
}

template<typename T>
template<typename ParamType>
inline Vec3<T>& Vec3<T>::operator*=( ParamType val )
{
	T coeff = static_cast<T>( val );
	m_val[0] *= coeff; m_val[1] *= coeff; m_val[2] *= coeff;
	return *this;
}

template<typename T>
template<typename ParamType>
inline Vec3<T>& Vec3<T>::operator/=( ParamType val )
{
	T coeff = static_cast<T>( val );
	m_val[0] /= coeff; m_val[1] /= coeff; m_val[2] /= coeff;
	return *this;
}

template<typename T>
inline T& Vec3<T>::operator[]( std::size_t ind )
{
	return m_val[ind];
}

template<typename T>
inline const T& Vec3<T>::operator[]( std::size_t ind ) const
{
	return m_val[ind];
}

template<typename T>
inline Vec3<T> operator+( const Vec3<T>& lhs, const Vec3<T>& rhs )
{
	return Vec3<T>( lhs.m_val[0] + rhs.m_val[0], lhs.m_val[1] + rhs.m_val[1], lhs.m_val[2] + rhs.m_val[2] );
}

template<typename T>
inline Vec3<T> operator-( const Vec3<T>& lhs, const Vec3<T>& rhs )
{
	return Vec3<T>( lhs.m_val[0] - rhs.m_val[0], lhs.m_val[1] - rhs.m_val[1], lhs.m_val[2] - rhs.m_val[2] );
}

template<typename T, typename ParamType>
inline Vec3<T> operator+( const Vec3<T>& lhs, ParamType rhs )
{
	T val = static_cast<T>( rhs );
	return Vec3<T>( lhs.m_val[0] + val, lhs.m_val[1] + val, lhs.m_val[2] + val );
}

template<typename T, typename ParamType>
inline Vec3<T> operator+( ParamType lhs, const Vec3<T>& rhs )
{
	return ( rhs + lhs );
}

template<typename T, typename ParamType>
inline Vec3<T> operator*( const Vec3<T>& lhs, ParamType rhs )
{
	T val = static_cast<T>( rhs );
	return Vec3<T>( lhs.m_val[0] * val, lhs.m_val[1] * val, lhs.m_val[2] * val );
}

template<typename T, typename ParamType>
inline Vec3<T> operator*( T lhs, const Vec3<T>& rhs )
{
	return ( rhs * lhs );
}

template<typename T, typename ParamType>
inline Vec3<T> operator/( const Vec3<T>& lhs, const ParamType& rhs )
{
	T val = static_cast<T>( rhs );
	return Vec3<T>( lhs.m_val[0] / val, lhs.m_val[1] / val, lhs.m_val[2] / val );
}

template<typename T, typename ParamType>
inline Vec3<T> operator/( const T& lhs, const Vec3<T>& rhs )
{
	return ( lhs / rhs );
}

// dot production
template<typename T>
inline T operator&( const Vec3<T>& lhs, const Vec3<T>& rhs )
{
	return lhs.m_val[0] * rhs.m_val[0] + lhs.m_val[1] * rhs.m_val[1] + lhs.m_val[2] * rhs.m_val[2];
}

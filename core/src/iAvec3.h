/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

/**	\class Vec3.
\brief Class representing 3 dimensional vector.
*/
class open_iA_Core_API iAVec3
{
public:
	float x,y,z;
	iAVec3(){}
	iAVec3(float px, float py, float pz)
	{
		x = px;
		y = py;
		z = pz;
	}
	iAVec3(float val)
	{
		x = val;
		y = val;
		z = val;
	}
	iAVec3(float data[3])
	{
		x = data[0];
		y = data[1];
		z = data[2];
	}
	iAVec3(const iAVec3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	iAVec3& operator= (const iAVec3& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}
	iAVec3 operator+ () const
	{
		return *this;
	}
	iAVec3 operator- () const
	{
		return iAVec3(-x,-y,-z);
	}
	iAVec3& operator+= (const iAVec3& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	iAVec3& operator-= (const iAVec3& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	iAVec3& operator*= (const iAVec3& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	iAVec3& operator*= (float f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}
	iAVec3& operator/= (const iAVec3& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}
	const float& operator[] (int index) const
	{
		return *(index+&x);
	}
	float& operator[] (int index)
	{
		return *(index+&x);
	}
	int     operator== (const iAVec3& v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}
	int	    operator!= (const iAVec3& v) const
	{
		return x != v.x || y != v.y || z != v.z;
	}
	int	    operator<  (const iAVec3& v) const
	{
		return ( x < v.x ) || ((x == v.x) && (y < v.y));
	}
	int	    operator>  (const iAVec3& v) const
	{
		return ( x > v.x ) || ((x == v.x) && (y > v.y));
	}
	float	length() const;
	static float angle(iAVec3 const & a, iAVec3 const & b);

	friend iAVec3 operator + (const iAVec3&,const iAVec3&);
	friend iAVec3 operator - (const iAVec3&,const iAVec3&);
	friend iAVec3 operator * (const iAVec3&,const iAVec3&);
	friend iAVec3 operator * (float        ,const iAVec3&);
	friend iAVec3 operator * (const iAVec3&,float);
	friend iAVec3 operator / (const iAVec3&,float);
	friend iAVec3 operator / (const iAVec3&,const iAVec3&);
	friend float  operator & (const iAVec3&,const iAVec3&);
	friend iAVec3 operator ^ (const iAVec3&,const iAVec3&);
};
inline iAVec3 operator + (const iAVec3& u,const iAVec3& v)
{
	return iAVec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

inline iAVec3 operator - (const iAVec3& u,const iAVec3& v)
{
	return iAVec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

inline iAVec3 operator * (const iAVec3& u,const iAVec3& v)
{
	return iAVec3(u.x * v.x, u.y * v.y, u.z * v.z);
}

inline iAVec3 operator * (const iAVec3& v,float a)
{
	return iAVec3(v.x * a, v.y * a, v.z * a);
}

inline iAVec3 operator * (float a, const iAVec3& v)
{
	return iAVec3(v.x * a, v.y * a, v.z * a);
}

inline iAVec3 operator / (const iAVec3& v,float a)
{
	return iAVec3(v.x / a, v.y / a, v.z / a);
}

inline iAVec3 operator / (const iAVec3& u,const iAVec3& v)
{
	return iAVec3(u.x / v.x, u.y / v.y, u.z / v.z);
}
//dot
inline float    operator & (const iAVec3& u,const iAVec3& v)
{
	return u.x*v.x + u.y*v.y + u.z*v.z;
}
//cross
inline iAVec3 operator ^ (const iAVec3& u,const iAVec3& v)
{
	return iAVec3(u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x);
}

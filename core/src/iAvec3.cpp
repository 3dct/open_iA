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
 
#include "pch.h"
#include "iAvec3.h"

#include <cmath>

iAVec3::iAVec3()
{
	x = 0;
	y = 0;
	z = 0;
}
iAVec3::iAVec3(float px, float py, float pz)
{
	x = px;
	y = py;
	z = pz;
}
iAVec3::iAVec3(float val)
{
	x = val;
	y = val;
	z = val;
}
iAVec3::iAVec3(float data[3])
{
	x = data[0];
	y = data[1];
	z = data[2];
}
iAVec3::iAVec3(const iAVec3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
}

float iAVec3::length() const
{
	return (float)sqrt(x*x + y*y + z*z);
}

float iAVec3::angle(iAVec3 const & a, iAVec3 const & b)
{
	return std::acos((a & b) / (a.length() * b.length()));
}

iAVec3& iAVec3::operator= (const iAVec3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}

iAVec3 iAVec3::operator+ () const
{
	return *this;
}

iAVec3 iAVec3::operator- () const
{
	return iAVec3(-x, -y, -z);
}

iAVec3& iAVec3::operator+= (const iAVec3& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

iAVec3& iAVec3::operator-= (const iAVec3& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

iAVec3& iAVec3::operator*= (const iAVec3& v)
{
	x *= v.x;
	y *= v.y;
	z *= v.z;
	return *this;
}

iAVec3& iAVec3::operator*= (float f)
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}

iAVec3& iAVec3::operator/= (const iAVec3& v)
{
	x /= v.x;
	y /= v.y;
	z /= v.z;
	return *this;
}

const float& iAVec3::operator[] (int index) const
{
	return *(index + &x);
}

float& iAVec3::operator[] (int index)
{
	return *(index + &x);
}

int iAVec3::operator== (const iAVec3& v) const
{
	return x == v.x && y == v.y && z == v.z;
}

int	iAVec3::operator!= (const iAVec3& v) const
{
	return x != v.x || y != v.y || z != v.z;
}

int	iAVec3::operator<  (const iAVec3& v) const
{
	return (x < v.x) || ((x == v.x) && (y < v.y));
}

int	iAVec3::operator>  (const iAVec3& v) const
{
	return (x > v.x) || ((x == v.x) && (y > v.y));
}

iAVec3 iAVec3::normalize() const
{
	return (*this) / this->length();
}

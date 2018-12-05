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
#include "iAmat4.h"

#include <cmath>

iAMat4::iAMat4 ( float v )
{
	for ( int i = 0; i < 4; i++)
		for ( int j = 0; j < 4; j++)
			x [i][j] = (i == j) ? v : 0.0f;

	x [3][3] = 1;
}

void iAMat4::invert ()
{
	iAMat4 out ( 1 );

	for ( int i = 0; i < 4; i++ )
	{
		float	d = x [i][i];

		if ( d != 1.0)
		{
			for ( int j = 0; j < 4; j++ )
			{
				out.x [i][j] /= d;
				x [i][j]     /= d;
			}
		}

		for ( int j = 0; j < 4; j++ )
		{
			if ( j != i )
			{
				if ( x [j][i] != 0.0)
				{
					float	mulBy = x[j][i];

					for ( int k = 0; k < 4; k++ )
					{
						x [j][k]     -= mulBy * x [i][k];
						out.x [j][k] -= mulBy * out.x [i][k];
					}
				}
			}
		}
	}

	*this = out;
}

void iAMat4::transpose()
{
	for ( int i = 0; i < 4; i++ )
		for ( int j = i; j < 4; j++ )
			if ( i != j )
				std::swap(x[i][j], x[j][i]);
}

iAMat4& iAMat4::operator += ( const iAMat4& a )
{
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			x [i][j] += a.x [i][j];

	return *this;
}

iAMat4& iAMat4::operator -= ( const iAMat4& a )
{
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			x [i][j] -= a.x [i][j];

	return *this;
}

iAMat4& iAMat4::operator *= ( float v )
{
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			x [i][j] *= v;

	return *this;
}

iAMat4& iAMat4::operator *= ( const iAMat4& a )
{
	iAMat4	res ( *this );
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
		{
			float sum = 0;
			for ( int k = 0; k < 4; k++ )
				sum += res.x [i][k] * a.x [k][j];
			x [i][j] = sum;
		}
	return *this;
}

iAMat4 operator + ( const iAMat4& a, const iAMat4& b )
{
	iAMat4 res;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			res.x [i][j] = a.x [i][j] + b.x [i][j];
	return res;
}

iAMat4 operator - ( const iAMat4& a, const iAMat4& b )
{
	iAMat4	res;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			res.x [i][j] = a.x [i][j] - b.x [i][j];
	return res;
}

iAMat4 operator * ( const iAMat4& a, const iAMat4& b )
{
	iAMat4	res;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
		{
			float sum = 0;
			for ( int k = 0; k < 4; k++ )
				sum += a.x [i][k] * b.x [k][j];
			res.x [i][j] = sum;
		}
	return res;
}

iAMat4 operator * ( const iAMat4& a, float v )
{
	iAMat4 res;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			res.x [i][j] = a.x [i][j] * v;

	return res;
}

iAMat4 operator * ( float v, const iAMat4& a )
{
	iAMat4 res;
	for ( int i = 0; i < 4; i++ )
		for ( int j = 0; j < 4; j++ )
			res.x [i][j] = a.x [i][j] * v;
	return res;
}

iAVec3 operator * ( const iAMat4& m, const iAVec3& v )
{
	iAVec3 res;

	res.x = m.x [0][0] * v.x + m.x [0][1] * v.y + m.x [0][2] * v.z + m.x [0][3];
	res.y = m.x [1][0] * v.x + m.x [1][1] * v.y + m.x [1][2] * v.z + m.x [1][3];
	res.z = m.x [2][0] * v.x + m.x [2][1] * v.y + m.x [2][2] * v.z + m.x [2][3];

	float	denom = m.x [3][0] * v.x + m.x [3][1] * v.y +  m.x [3][2] * v.z + m.x [3][3];

	if ( denom != 1.0 )
		res = res / denom;

	return res;
}

//////////////////////// Derived functions /////////////////////////////

iAMat4 translate ( const iAVec3& loc )
{
	iAMat4 res ( 1 );
	res.x [0][3] = loc.x;
	res.x [1][3] = loc.y;
	res.x [2][3] = loc.z;
	return res;
}

iAMat4 scale ( const iAVec3& v )
{
	iAMat4 res ( 1 );
	res.x [0][0] = v.x;
	res.x [1][1] = v.y;
	res.x [2][2] = v.z;
	return res;
}

iAMat4 rotateX ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = cos ( angle );
	float  sine   = sin ( angle );

	res.x [1][1] = cosine;
	res.x [1][2] = -sine;
	res.x [2][1] = sine;
	res.x [2][2] = cosine;

	return res;
}

iAMat4 rotateY ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = cos ( angle );
	float  sine   = sin ( angle );

	res.x [0][0] = cosine;
	res.x [0][2] = -sine;
	res.x [2][0] = sine;
	res.x [2][2] = cosine;

	return res;
}

iAMat4 rotateZ ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = cos ( angle );
	float  sine   = sin ( angle );

	res.x [0][0] = cosine;
	res.x [0][1] = -sine;
	res.x [1][0] = sine;
	res.x [1][1] = cosine;

	return res;
}

iAMat4 rotation ( const iAVec3& axis, float angle )
{
	iAMat4 res ( 1 );
	float  cosine = cos ( angle );
	float  sine   = sin ( angle );

	res.x [0][0] = axis.x * axis.x + ( 1 - axis.x * axis.x ) * cosine;
	res.x [1][0] = axis.x * axis.y * ( 1 - cosine ) + axis.z * sine;
	res.x [2][0] = axis.x * axis.z * ( 1 - cosine ) - axis.y * sine;
	res.x [3][0] = 0;

	res.x [0][1] = axis.x * axis.y * ( 1 - cosine ) - axis.z * sine;
	res.x [1][1] = axis.y * axis.y + ( 1 - axis.y * axis.y ) * cosine;
	res.x [2][1] = axis.y * axis.z * ( 1 - cosine ) + axis.x * sine;
	res.x [3][1] = 0;

	res.x [0][2] = axis.x * axis.z * ( 1 - cosine ) + axis.y * sine;
	res.x [1][2] = axis.y * axis.z * ( 1 - cosine ) - axis.x * sine;
	res.x [2][2] = axis.z * axis.z + ( 1 - axis.z * axis.z ) * cosine;
	res.x [3][2] = 0;

	res.x [0][3] = 0;
	res.x [1][3] = 0;
	res.x [2][3] = 0;
	res.x [3][3] = 1;

	return res;
}

iAMat4 rotationX ( float angle )
{
	iAVec3 axis = iAVec3(1,0,0);
	return rotation(axis, angle);
}

iAMat4 rotationY ( float angle )
{
	iAVec3 axis = iAVec3(0,1,0);
	return rotation(axis, angle);
}

iAMat4 rotationZ ( float angle )
{
	iAVec3 axis = iAVec3(0,0,1);
	return rotation(axis, angle);
}

iAMat4 mirrorX ()
{
	iAMat4 res ( 1 );
	res.x [0][0] = -1;
	return res;
}

iAMat4 mirrorY ()
{
	iAMat4 res ( 1 );
	res.x [1][1] = -1;
	return res;
}

iAMat4 mirrorZ ()
{
	iAMat4 res ( 1 );
	res.x [2][2] = -1;
	return res;
}

iAMat4 orthoProjectYZ ()
{
	iAMat4 res ( 1 );
	res.x [0][0] = 0;
	return res;
}

iAMat4 orthoProjectXY ()
{
	iAMat4 res ( 1 );
	res.x [2][2] = 0;
	return res;
}

iAMat4 orthoProjectXZ ()
{
	iAMat4 res ( 1 );
	res.x [1][1] = 0;
	return res;
}

iAMat4	axProjectYZ (iAVec3& v)
{
	iAMat4 res ( 1 );
	res.x [2][2] = v.z/v.x;
	res.x [1][2] = v.y/v.x;
	return res;
}

iAMat4 axProjectXY (iAVec3& v)
{
	iAMat4 res ( 1 );
	res.x [0][2] = v.x/v.z;
	res.x [1][2] = v.y/v.z;
	return res;
}

iAMat4 axProjectXZ (iAVec3& v)
{
	iAMat4 res ( 1 );
	res.x [0][2] = v.x/v.y;
	res.x [2][2] = v.z/v.y;
	return res;
}

iAMat4 frProjectXY1(float focus)
{
	iAMat4 res ( 1 );
	res.x[2][2] = 0;
	res.x[3][2] = -1/focus;
	return res;
}
iAMat4 frProjectYZ1(float focus)
{
	iAMat4 res ( 1 );
	res.x [0][0] = 0;
	res.x[3][2] = -1/focus;
	return res;
}
iAMat4 frProjectXZ1(float focus)
{
	iAMat4 res ( 1 );
	res.x [1][1] = 0;
	res.x[3][2] = -1/focus;
	return res;
}

iAMat4 frProjectXY2(float focus)
{
	iAMat4 res ( 1 );
	res.x[2][2] = 0;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}
iAMat4 frProjectYZ2(float focus)
{
	iAMat4 res ( 1 );
	res.x [0][0] = 0;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}
iAMat4 frProjectXZ2(float focus)
{
	iAMat4 res ( 1 );
	res.x [1][1] = 0;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}

iAMat4 frProjectXY3(float focus)
{
	iAMat4 res ( 1 );
	res.x[2][2] = 0;
	res.x[3][0] = -1/focus;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}

iAMat4 frProjectYZ3(float focus)
{
	iAMat4 res ( 1 );
	res.x [0][0] = 0;
	res.x[3][0] = -1/focus;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}

iAMat4 frProjectXZ3(float focus)
{
	iAMat4 res ( 1 );
	res.x [1][1] = 0;
	res.x[3][0] = -1/focus;
	res.x[3][1] = -1/focus;
	res.x[3][2] = -1/focus;
	return res;
}

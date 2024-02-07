// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAmat4.h"

#include <utility>
#include <cmath>

iAMat4::iAMat4(float v)
{
	for (int i = 0; i < Rows; i++)
	{
		for (int j = 0; j < Cols; j++)
		{
			x[i][j] = (i == j) ? v : 0.0f;
		}
	}
	x [3][3] = 1;
}

iAMat4::iAMat4(const iAMat4& m)
{
	std::copy(&m.x[0][0], &m.x[0][0] + Rows*Cols, &x[0][0]);
}

iAMat4& iAMat4::operator=(iAMat4 const & m)
{
	std::copy(&m.x[0][0], &m.x[0][0] + Rows * Cols, &x[0][0]);
	return *this;
}

void iAMat4::invert ()
{
	iAMat4 out ( 1 );

	for ( int i = 0; i < 4; i++ )
	{
		float d = x [i][i];
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
	for (int i = 0; i < 4; i++)
	{
		for (int j = i; j < 4; j++)
		{
			if (i != j)
			{
				std::swap(x[i][j], x[j][i]);
			}
		}
	}
}

iAMat4& iAMat4::operator += ( const iAMat4& a )
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			x[i][j] += a.x[i][j];
		}
	}

	return *this;
}

iAMat4& iAMat4::operator -= ( const iAMat4& a )
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			x[i][j] -= a.x[i][j];
		}
	}

	return *this;
}

iAMat4& iAMat4::operator *= ( float v )
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			x[i][j] *= v;
		}
	}

	return *this;
}

iAMat4& iAMat4::operator *= ( const iAMat4& a )
{
	iAMat4 res ( *this );
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			float sum = 0;
			for (int k = 0; k < 4; k++)
			{
				sum += res.x[i][k] * a.x[k][j];
			}
			x[i][j] = sum;
		}
	}
	return *this;
}

iAMat4 operator + ( const iAMat4& a, const iAMat4& b )
{
	iAMat4 res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			res.x[i][j] = a.x[i][j] + b.x[i][j];
		}
	}
	return res;
}

iAMat4 operator - ( const iAMat4& a, const iAMat4& b )
{
	iAMat4 res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			res.x[i][j] = a.x[i][j] - b.x[i][j];
		}
	}
	return res;
}

iAMat4 operator * ( const iAMat4& a, const iAMat4& b )
{
	iAMat4 res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			float sum = 0;
			for (int k = 0; k < 4; k++)
			{
				sum += a.x[i][k] * b.x[k][j];
			}
			res.x[i][j] = sum;
		}
	}
	return res;
}

iAMat4 operator * ( const iAMat4& a, float v )
{
	iAMat4 res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			res.x[i][j] = a.x[i][j] * v;
		}
	}
	return res;
}

iAMat4 operator * ( float v, const iAMat4& a )
{
	iAMat4 res;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			res.x[i][j] = a.x[i][j] * v;
		}
	}
	return res;
}

iAVec3f operator * ( const iAMat4& m, const iAVec3f & v )
{
	iAVec3f res(
		m.x[0][0] * v.x() + m.x[0][1] * v.y() + m.x[0][2] * v.z() + m.x[0][3],
		m.x[1][0] * v.x() + m.x[1][1] * v.y() + m.x[1][2] * v.z() + m.x[1][3],
		m.x[2][0] * v.x() + m.x[2][1] * v.y() + m.x[2][2] * v.z() + m.x[2][3]
	);
	float denom = m.x [3][0] * v.x() + m.x [3][1] * v.y() +  m.x [3][2] * v.z() + m.x [3][3];

	if (denom != 1.0)
	{
		res = res / denom;
	}

	return res;
}

//////////////////////// Derived functions /////////////////////////////

iAMat4 translate ( const iAVec3f & loc )
{
	iAMat4 res ( 1 );
	res.x [0][3] = loc.x();
	res.x [1][3] = loc.y();
	res.x [2][3] = loc.z();
	return res;
}

iAMat4 scale ( const iAVec3f & v )
{
	iAMat4 res ( 1 );
	res.x [0][0] = v.x();
	res.x [1][1] = v.y();
	res.x [2][2] = v.z();
	return res;
}

/*
iAMat4 rotateX ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = std::cos ( angle );
	float  sine   = std::sin ( angle );

	res.x [1][1] = cosine;
	res.x [1][2] = -sine;
	res.x [2][1] = sine;
	res.x [2][2] = cosine;

	return res;
}

iAMat4 rotateY ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = std::cos ( angle );
	float  sine   = std::sin ( angle );

	res.x [0][0] = cosine;
	res.x [0][2] = -sine;
	res.x [2][0] = sine;
	res.x [2][2] = cosine;

	return res;
}

iAMat4 rotateZ ( float angle )
{
	iAMat4 res ( 1 );
	float  cosine = std::cos ( angle );
	float  sine   = std::sin ( angle );

	res.x [0][0] = cosine;
	res.x [0][1] = -sine;
	res.x [1][0] = sine;
	res.x [1][1] = cosine;

	return res;
}
*/

iAMat4 rotation ( const iAVec3f & axis, float angle )
{
	iAMat4 res ( 1 );
	float  cosine = std::cos ( angle );
	float  sine   = std::sin ( angle );

	res.x [0][0] = axis.x() * axis.x() + ( 1 - axis.x() * axis.x() ) * cosine;
	res.x [1][0] = axis.x() * axis.y() * ( 1 - cosine ) + axis.z() * sine;
	res.x [2][0] = axis.x() * axis.z() * ( 1 - cosine ) - axis.y() * sine;
	res.x [3][0] = 0;

	res.x [0][1] = axis.x() * axis.y() * ( 1 - cosine ) - axis.z() * sine;
	res.x [1][1] = axis.y() * axis.y() + ( 1 - axis.y() * axis.y() ) * cosine;
	res.x [2][1] = axis.y() * axis.z() * ( 1 - cosine ) + axis.x() * sine;
	res.x [3][1] = 0;

	res.x [0][2] = axis.x() * axis.z() * ( 1 - cosine ) + axis.y() * sine;
	res.x [1][2] = axis.y() * axis.z() * ( 1 - cosine ) - axis.x() * sine;
	res.x [2][2] = axis.z() * axis.z() + ( 1 - axis.z() * axis.z() ) * cosine;
	res.x [3][2] = 0;

	res.x [0][3] = 0;
	res.x [1][3] = 0;
	res.x [2][3] = 0;
	res.x [3][3] = 1;

	return res;
}

iAMat4 rotationX ( float angle )
{
	iAVec3f axis(1,0,0);
	return rotation(axis, angle);
}

iAMat4 rotationY ( float angle )
{
	iAVec3f axis(0,1,0);
	return rotation(axis, angle);
}

iAMat4 rotationZ ( float angle )
{
	iAVec3f axis(0,0,1);
	return rotation(axis, angle);
}
/*

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

iAMat4 axProjectYZ (iAVec3f & v)
{
	iAMat4 res ( 1 );
	res.x [2][2] = v.z()/v.x();
	res.x [1][2] = v.y()/v.x();
	return res;
}

iAMat4 axProjectXY (iAVec3f & v)
{
	iAMat4 res ( 1 );
	res.x [0][2] = v.x()/v.z();
	res.x [1][2] = v.y()/v.z();
	return res;
}

iAMat4 axProjectXZ (iAVec3f & v)
{
	iAMat4 res ( 1 );
	res.x [0][2] = v.x()/v.y();
	res.x [2][2] = v.z()/v.y();
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
*/

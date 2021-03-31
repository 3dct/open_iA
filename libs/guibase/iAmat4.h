/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAVec3.h"
#include "iAguibase_export.h"

//! Class representing a 4x4 float matrix.
class iAguibase_API iAMat4
{
public:
	static const int Rows = 4;
	static const int Cols = 4;
	float x [Rows][Cols];

	//! Create a matrix (with uninitialized values!).
	iAMat4 () {}
	//! Create a matrix with the given value in the diagonal places (0-2), 1 in 3rd diagonal place, and 0 in the other places.
	//! Example: iAMat4(0.5) would give
	//! 0.5 0   0   0
	//! 0   0.5 0   0
	//! 0   0   0.5 0
	//! 0   0   0   1
	explicit iAMat4 (float v);
	//! Create new matrix, copying given matrix values.
	iAMat4(const iAMat4& m);
	//! Assign values from existing matrix.
	iAMat4& operator= (iAMat4 const& m);

	iAMat4& operator += ( const iAMat4& );
	iAMat4& operator -= ( const iAMat4& );
	iAMat4& operator *= ( const iAMat4& );
	iAMat4& operator *= ( float );
	iAMat4& operator /= ( float );

	void	invert ();
	void	transpose ();

	friend	iAMat4   operator + ( const iAMat4&, const iAMat4& );
	friend	iAMat4   operator - ( const iAMat4&, const iAMat4& );
	friend	iAMat4   operator * ( const iAMat4&, float );
	friend  iAMat4   operator * ( float,         const iAMat4& );
	friend	iAguibase_API iAMat4   operator * ( const iAMat4&, const iAMat4& );
	friend	iAguibase_API iAVec3f operator * ( const iAMat4&, const iAVec3f& );
};

//iAVec3f operator * (const iAMat4 &, const iAVec3f &);

iAguibase_API iAMat4	translate ( const iAVec3f& );
iAguibase_API iAMat4	scale     ( const iAVec3f& );
iAguibase_API iAMat4	rotation  ( const iAVec3f& v, float );
iAguibase_API iAMat4	rotationX   ( float );
iAguibase_API iAMat4	rotationY   ( float );
iAguibase_API iAMat4	rotationZ   ( float );
/*
iAMat4	rotateX   ( float );
iAMat4	rotateY   ( float );
iAMat4	rotateZ   ( float );
iAMat4	mirrorX   ();
iAMat4	mirrorY   ();
iAMat4	mirrorZ   ();
iAMat4	orthoProjectYZ();
iAMat4	orthoProjectXZ();
iAMat4	orthoProjectXY();
iAMat4	axProjectYZ(iAVec3f& );
iAMat4	axProjectXZ(iAVec3f& );
iAMat4	axProjectXY(iAVec3f& );
iAMat4  frProjectXY1(float);
iAMat4  frProjectXZ1(float);
iAMat4  frProjectYZ1(float);
iAMat4  frProjectXY2(float);
iAMat4  frProjectXZ2(float);
iAMat4  frProjectYZ2(float);
iAMat4  frProjectXY3(float);
iAMat4  frProjectXZ3(float);
iAMat4  frProjectYZ3(float);
*/
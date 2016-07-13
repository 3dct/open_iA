/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "iAvec3.h"
#include "open_iA_Core_export.h"

#include <memory.h>

/**	\class Mat4.
	\brief Class representing 4x4 float matrix.
*/
class open_iA_Core_API iAMat4
{
public:
	float x [4][4];

	iAMat4 () {}
	iAMat4 ( float );
	iAMat4 ( const iAMat4& m )
	{
		memcpy ( & x [0][0], &m.x [0][0], 16*sizeof ( float ) );
	}

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
	friend	open_iA_Core_API iAMat4   operator * ( const iAMat4&, const iAMat4& );
	friend	open_iA_Core_API iAVec3 operator * ( const iAMat4&, const iAVec3& );
};

//iAVec3 operator * (const iAMat4&, const iAVec3&);

open_iA_Core_API iAMat4	translate ( const iAVec3& );
open_iA_Core_API iAMat4	scale     ( const iAVec3& );
iAMat4	rotateX   ( float );
iAMat4	rotateY   ( float );
iAMat4	rotateZ   ( float );
open_iA_Core_API iAMat4	rotation  ( const iAVec3& v, float );
iAMat4	rotateX   ( float );
iAMat4	rotateY   ( float );
iAMat4	rotateZ   ( float );
open_iA_Core_API iAMat4	rotationX   ( float );
open_iA_Core_API iAMat4	rotationY   ( float );
open_iA_Core_API iAMat4	rotationZ   ( float );
iAMat4	mirrorX   ();
iAMat4	mirrorY   ();
iAMat4	mirrorZ   ();
iAMat4	orthoProjectYZ();
iAMat4	orthoProjectXZ();
iAMat4	orthoProjectXY();
iAMat4	axProjectYZ(iAVec3& );
iAMat4	axProjectXZ(iAVec3& );
iAMat4	axProjectXY(iAVec3& );
iAMat4  frProjectXY1(float);
iAMat4  frProjectXZ1(float);
iAMat4  frProjectYZ1(float);
iAMat4  frProjectXY2(float);
iAMat4  frProjectXZ2(float);
iAMat4  frProjectYZ2(float);
iAMat4  frProjectXY3(float);
iAMat4  frProjectXZ3(float);
iAMat4  frProjectYZ3(float);

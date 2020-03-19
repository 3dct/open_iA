/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <vtkType.h>

// this allows us to get the correct VTK_* data type for a built-in type
template <typename T> struct iAVtkDataType                 { static const int value = VTK_VOID;           };
template <>           struct iAVtkDataType<char>           { static const int value = VTK_CHAR;           };
template <>           struct iAVtkDataType<unsigned char>  { static const int value = VTK_UNSIGNED_CHAR;  };
template <>           struct iAVtkDataType<short>          { static const int value = VTK_SHORT;          };
template <>           struct iAVtkDataType<unsigned short> { static const int value = VTK_UNSIGNED_SHORT; };
template <>           struct iAVtkDataType<int>            { static const int value = VTK_INT;            };
template <>           struct iAVtkDataType<unsigned int>   { static const int value = VTK_UNSIGNED_INT;   };
template <>           struct iAVtkDataType<long>           { static const int value = VTK_LONG;           };
template <>           struct iAVtkDataType<unsigned long>  { static const int value = VTK_UNSIGNED_LONG;  };
template <>           struct iAVtkDataType<long long>      { static const int value = VTK_LONG_LONG;      };
template <>           struct iAVtkDataType<unsigned long long>{ static const int value = VTK_UNSIGNED_LONG_LONG;};
template <>           struct iAVtkDataType<float>          { static const int value = VTK_FLOAT;          };
template <>           struct iAVtkDataType<double>         { static const int value = VTK_DOUBLE;         };
//not supported on g++ (4.6, without --std=c++0x)
//template <>           struct VtkDataType<__int64>        { static const int value = VTK___INT64;        };

/*
// This would allow us to get the correct primitive type for an int representing a VTK_* type
// but only if the VTK_* type is known at compile time
template <int x>      struct PrimitiveType                    { };
template <>           struct PrimitiveType<VTK_CHAR>          { typedef char           Type; };
template <>           struct PrimitiveType<VTK_UNSIGNED_CHAR> { typedef unsigned char  Type; };
template <>           struct PrimitiveType<VTK_SHORT>         { typedef short          Type; };
template <>           struct PrimitiveType<VTK_UNSIGNED_SHORT>{ typedef unsigned short Type; };
template <>           struct PrimitiveType<VTK_INT>           { typedef int            Type; };
template <>           struct PrimitiveType<VTK_UNSIGNED_INT>  { typedef unsigned int   Type; };
template <>           struct PrimitiveType<VTK_LONG>          { typedef long           Type; };
template <>           struct PrimitiveType<VTK_UNSIGNED_LONG> { typedef unsigned long  Type; };
template <>           struct PrimitiveType<VTK_FLOAT>         { typedef float          Type; };
template <>           struct PrimitiveType<VTK_DOUBLE>        { typedef double         Type; };
template <>           struct PrimitiveType<VTK___INT64>       { typedef __int64        Type; };
*/

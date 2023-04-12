// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkType.h>

//! Mapper from built-in types to the correct VTK_* data type value
template <typename T> struct iAVtkDataType                 { static const int value = VTK_VOID;           };
//! Maps type char to VKT_SIGNED_CHAR
template <>           struct iAVtkDataType<char>           { static const int value = VTK_SIGNED_CHAR;    };
//! Maps type unsigned char to VKT_UNSIGNED_CHAR
template <>           struct iAVtkDataType<unsigned char>  { static const int value = VTK_UNSIGNED_CHAR;  };
//! Maps type short to VKT_SHORT
template <>           struct iAVtkDataType<short>          { static const int value = VTK_SHORT;          };
//! Maps type unsigned short to VKT_UNSIGNED_SHORT
template <>           struct iAVtkDataType<unsigned short> { static const int value = VTK_UNSIGNED_SHORT; };
//! Maps type int to VKT_INT
template <>           struct iAVtkDataType<int>            { static const int value = VTK_INT;            };
//! Maps type unsigned int to VKT_UNSIGNED_INT
template <>           struct iAVtkDataType<unsigned int>   { static const int value = VTK_UNSIGNED_INT;   };
//! Maps type long to VKT_UNSIGNED_LONG
template <>           struct iAVtkDataType<long>           { static const int value = VTK_LONG;           };
//! Maps type unsigned long to VKT_UNSIGNED_LONG
template <>           struct iAVtkDataType<unsigned long>  { static const int value = VTK_UNSIGNED_LONG;  };
//! Maps type long long to VKT_LONG_LONG
template <>           struct iAVtkDataType<long long>      { static const int value = VTK_LONG_LONG;      };
//! Maps type unsigned long long to VKT_UNSIGNED_LONG_LONG
template <>           struct iAVtkDataType<unsigned long long>{ static const int value = VTK_UNSIGNED_LONG_LONG;};
//! Maps type float to VKT_FLOAT
template <>           struct iAVtkDataType<float>          { static const int value = VTK_FLOAT;          };
//! Maps type double to VKT_DOUBLE
template <>           struct iAVtkDataType<double>         { static const int value = VTK_DOUBLE;         };
//not supported on g++ (4.6, without --std=c++0x)
//template <>           struct VtkDataType<__int64>        { static const int value = VTK___INT64;        };

/*
// This would allow us to get the correct primitive type for an int representing a VTK_* type
// but only if the VTK_* type is known at compile time
template <int x>      struct PrimitiveType                    { };
template <>           struct PrimitiveType<VTK_CHAR>          { typedef char           Type; };
template <>           struct PrimitiveType<VTK_SIGNED_CHAR>   { typedef char           Type; };
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

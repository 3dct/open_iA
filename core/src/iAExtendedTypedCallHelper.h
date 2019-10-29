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

#include <itkConfigure.h>    // for ITK_VERSION...

#include <QString>
// Requirements:
// |function| return type must be void
//
// Usage:
//
// having for instance such |type| variable:
//   ColumnType type = INT;
// and such |foo| function definition:
//   template <ColumnType T>
//   void foo(t1 arg1, t2 arg2) {
//   }
//
// instead of writing (won't compile):
//   foo<type>(arg1, arg2);
// write this:
//   xxx_TYPED_CALL(foo, type, arg1, arg2);
//
//
// for foo with 0 arguments write this:
//   xxx_TYPED_CALL(foo, type, );
//
#if ITK_VERSION_MAJOR > 4 || (ITK_VERSION_MAJOR == 4 && ITK_VERSION_MINOR > 12)
#define ITK_EXTENDED_TYPED_CALL(function, itk_scalar_type, itk_image_type, ...) \
{                                                                           \
	if (itk_image_type == itk::ImageIOBase::SCALAR)                         \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<unsigned char>( __VA_ARGS__ );                     \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<char>( __VA_ARGS__ );                              \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<short>( __VA_ARGS__ );                             \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<unsigned short>( __VA_ARGS__ );                    \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<int>( __VA_ARGS__ );                               \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<unsigned int>( __VA_ARGS__ );                      \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<long>( __VA_ARGS__ );                              \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<unsigned long>( __VA_ARGS__ );                     \
				break;                                                      \
			case itk::ImageIOBase::LONGLONG:                                \
				function<long long>( __VA_ARGS__ );                         \
				break;                                                      \
			case itk::ImageIOBase::ULONGLONG:                               \
				function<unsigned long long>( __VA_ARGS__ );                \
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<float>( __VA_ARGS__ );                             \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<double>( __VA_ARGS__ );                            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toStdString().c_str() );                                \
		}                                                                   \
	}                                                                       \
	else if (itk_image_type == itk::ImageIOBase::RGB)                       \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<itk::RGBPixel< unsigned char >>( __VA_ARGS__ );    \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<itk::RGBPixel<char>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<itk::RGBPixel<short>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<itk::RGBPixel<unsigned short>>( __VA_ARGS__ );     \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<itk::RGBPixel<int>>( __VA_ARGS__ );                \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<itk::RGBPixel<unsigned int>>( __VA_ARGS__ );       \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<itk::RGBPixel<long>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<itk::RGBPixel<unsigned long>>( __VA_ARGS__ );      \
				break;                                                      \
			case itk::ImageIOBase::LONGLONG:                                \
				function<itk::RGBPixel<long long>>( __VA_ARGS__ );          \
				break;                                                      \
			case itk::ImageIOBase::ULONGLONG:                               \
				function<itk::RGBPixel<unsigned long long>>( __VA_ARGS__ ); \
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<itk::RGBPixel<float>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<itk::RGBPixel<double>>( __VA_ARGS__ );             \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toLatin1().data() );                                    \
		}                                                                   \
	}                                                                       \
	else if ( itk_image_type == itk::ImageIOBase::RGBA ||                   \
		 itk_image_type == itk::ImageIOBase::VECTOR )                       \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<itk::RGBAPixel< unsigned char >>( __VA_ARGS__ );   \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<itk::RGBAPixel<char>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<itk::RGBAPixel<short>>( __VA_ARGS__ );             \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<itk::RGBAPixel<unsigned short>>( __VA_ARGS__ );    \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<itk::RGBAPixel<int>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<itk::RGBAPixel<unsigned int>>( __VA_ARGS__ );      \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<itk::RGBAPixel<long>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<itk::RGBAPixel<unsigned long>>( __VA_ARGS__ );     \
				break;                                                      \
			case itk::ImageIOBase::LONGLONG:                                \
				function<itk::RGBAPixel<long long>>( __VA_ARGS__ );         \
				break;                                                      \
			case itk::ImageIOBase::ULONGLONG:                               \
				function<itk::RGBAPixel<unsigned long long>>( __VA_ARGS__ );\
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<itk::RGBAPixel<float>>( __VA_ARGS__ );             \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<itk::RGBAPixel<double>>( __VA_ARGS__ );            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toLatin1().data() );                                    \
		}                                                                   \
	}                                                                       \
	else                                                                    \
	{                                                                       \
		throw itk::ExceptionObject(__FILE__, __LINE__,                      \
			QString("Typed Call: Unknown pixel type.").toLatin1().data() ); \
	}                                                                       \
}
#else
// Since we cannot have an #if inside the macro definition,
// we need to specify the macro two times, one time with the
// long long types, one time without...
#define ITK_EXTENDED_TYPED_CALL(function, itk_scalar_type, itk_image_type, ...)\
{                                                                           \
	if (itk_image_type == itk::ImageIOBase::SCALAR)                         \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<unsigned char>( __VA_ARGS__ );                     \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<char>( __VA_ARGS__ );                              \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<short>( __VA_ARGS__ );                             \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<unsigned short>( __VA_ARGS__ );                    \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<int>( __VA_ARGS__ );                               \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<unsigned int>( __VA_ARGS__ );                      \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<long>( __VA_ARGS__ );                              \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<unsigned long>( __VA_ARGS__ );                     \
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<float>( __VA_ARGS__ );                             \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<double>( __VA_ARGS__ );                            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toStdString().c_str() );                                \
		}                                                                   \
	}                                                                       \
	else if (itk_image_type == itk::ImageIOBase::RGB)                       \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<itk::RGBPixel< unsigned char >>( __VA_ARGS__ );    \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<itk::RGBPixel<char>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<itk::RGBPixel<short>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<itk::RGBPixel<unsigned short>>( __VA_ARGS__ );     \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<itk::RGBPixel<int>>( __VA_ARGS__ );                \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<itk::RGBPixel<unsigned int>>( __VA_ARGS__ );       \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<itk::RGBPixel<long>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<itk::RGBPixel<unsigned long>>( __VA_ARGS__ );      \
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<itk::RGBPixel<float>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<itk::RGBPixel<double>>( __VA_ARGS__ );             \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toLatin1().data() );                                    \
		}                                                                   \
	}                                                                       \
	else if ( itk_image_type == itk::ImageIOBase::RGBA ||                   \
		 itk_image_type == itk::ImageIOBase::VECTOR )                       \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case itk::ImageIOBase::UCHAR:                                   \
				function<itk::RGBAPixel< unsigned char >>( __VA_ARGS__ );   \
				break;                                                      \
			case itk::ImageIOBase::CHAR:                                    \
				function<itk::RGBAPixel<char>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::SHORT:                                   \
				function<itk::RGBAPixel<short>>( __VA_ARGS__ );             \
				break;                                                      \
			case itk::ImageIOBase::USHORT:                                  \
				function<itk::RGBAPixel<unsigned short>>( __VA_ARGS__ );    \
				break;                                                      \
			case itk::ImageIOBase::INT:                                     \
				function<itk::RGBAPixel<int>>( __VA_ARGS__ );               \
				break;                                                      \
			case itk::ImageIOBase::UINT:                                    \
				function<itk::RGBAPixel<unsigned int>>( __VA_ARGS__ );      \
				break;                                                      \
			case itk::ImageIOBase::LONG:                                    \
				function<itk::RGBAPixel<long>>( __VA_ARGS__ );              \
				break;                                                      \
			case itk::ImageIOBase::ULONG:                                   \
				function<itk::RGBAPixel<unsigned long>>( __VA_ARGS__ );     \
				break;                                                      \
			case itk::ImageIOBase::FLOAT:                                   \
				function<itk::RGBAPixel<float>>( __VA_ARGS__ );             \
				break;                                                      \
			case itk::ImageIOBase::DOUBLE:                                  \
				function<itk::RGBAPixel<double>>( __VA_ARGS__ );            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString( "Typed Call: Unknown scalar pixel type." ).    \
					toLatin1().data() );                                    \
		}                                                                   \
	}                                                                       \
	else                                                                    \
	{                                                                       \
		throw itk::ExceptionObject(__FILE__, __LINE__,                      \
			QString("Typed Call: Unknown pixel type.").toLatin1().data() ); \
	}                                                                       \
}
#endif

#define VTK_EXTENDED_TYPED_CALL(function, vtk_scalar_type, number_of_components,...)     \
{                                                                                        \
	if ( number_of_components < 2 )                                                      \
	{                                                                                    \
		switch ( vtk_scalar_type )                                                       \
		{                                                                                \
			case VTK_UNSIGNED_CHAR:                                                      \
				function<unsigned char>( __VA_ARGS__ );                                  \
				break;                                                                   \
			case VTK_CHAR:                                                               \
				function<char>( __VA_ARGS__ );                                           \
				break;                                                                   \
			case VTK_SHORT:                                                              \
				function<short>( __VA_ARGS__ );                                          \
				break;                                                                   \
			case VTK_UNSIGNED_SHORT:                                                     \
				function<unsigned short>( __VA_ARGS__ );                                 \
				break;                                                                   \
			case VTK_INT:                                                                \
				function<int>( __VA_ARGS__ );                                            \
				break;                                                                   \
			case VTK_UNSIGNED_INT:                                                       \
				function<unsigned int>( __VA_ARGS__ );                                   \
				break;                                                                   \
			case VTK_LONG:                                                               \
				function<long>( __VA_ARGS__ );                                           \
				break;                                                                   \
			case VTK_UNSIGNED_LONG:                                                      \
				function<unsigned long>( __VA_ARGS__ );                                  \
				break;                                                                   \
			case VTK_FLOAT:                                                              \
				function<float>( __VA_ARGS__ );                                          \
				break;                                                                   \
			case VTK_DOUBLE:                                                             \
				function<double>( __VA_ARGS__ );                                         \
				break;                                                                   \
			default:                                                                     \
				throw itk::ExceptionObject( __FILE__, __LINE__,                          \
					QString( "Typed Call: Unknown component type." ).toLatin1().data() );\
		}                                                                                \
	}                                                                                    \
	else                                                                                 \
	{                                                                                    \
		switch ( vtk_scalar_type )                                                       \
		{                                                                                \
			case VTK_UNSIGNED_CHAR:                                                      \
				function < itk::RGBAPixel<unsigned char>>( __VA_ARGS__ );                \
				break;                                                                   \
			case VTK_CHAR:                                                               \
				function < itk::RGBAPixel<char>>( __VA_ARGS__ );                         \
				break;                                                                   \
			case VTK_SHORT:                                                              \
				function < itk::RGBAPixel<short>>( __VA_ARGS__ );                        \
				break;                                                                   \
			case VTK_UNSIGNED_SHORT:                                                     \
				function < itk::RGBAPixel<unsigned short>>( __VA_ARGS__ );               \
				break;                                                                   \
			case VTK_INT:                                                                \
				function < itk::RGBAPixel<int>>( __VA_ARGS__ );                          \
				break;                                                                   \
			case VTK_UNSIGNED_INT:                                                       \
				function < itk::RGBAPixel<unsigned int>>( __VA_ARGS__ );                 \
				break;                                                                   \
			case VTK_LONG:                                                               \
				function < itk::RGBAPixel<long>>( __VA_ARGS__ );                         \
				break;                                                                   \
			case VTK_UNSIGNED_LONG:                                                      \
				function < itk::RGBAPixel<unsigned long>>( __VA_ARGS__ );                \
				break;                                                                   \
			case VTK_FLOAT:                                                              \
				function < itk::RGBAPixel<float>>( __VA_ARGS__ );                        \
				break;                                                                   \
			case VTK_DOUBLE:                                                             \
				function < itk::RGBAPixel<double>>( __VA_ARGS__ );                       \
				break;                                                                   \
			default:                                                                     \
				throw itk::ExceptionObject( __FILE__, __LINE__,                          \
					QString( "Typed Call: Unknown pixel type." ).toLatin1().data() );    \
		}                                                                                \
	}                                                                                    \
}

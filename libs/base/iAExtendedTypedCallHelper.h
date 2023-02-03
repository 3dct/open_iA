// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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
#define ITK_EXTENDED_TYPED_CALL(function, itk_scalar_type, itk_pixel_type, ...) \
{                                                                           \
	if (itk_pixel_type == iAITKIO::PixelType::SCALAR)                       \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case iAITKIO::ScalarType::UCHAR:                                \
				function<unsigned char>( __VA_ARGS__ );                     \
				break;                                                      \
			case iAITKIO::ScalarType::CHAR:                                 \
				function<char>( __VA_ARGS__ );                              \
				break;                                                      \
			case iAITKIO::ScalarType::SHORT:                                \
				function<short>( __VA_ARGS__ );                             \
				break;                                                      \
			case iAITKIO::ScalarType::USHORT:                               \
				function<unsigned short>( __VA_ARGS__ );                    \
				break;                                                      \
			case iAITKIO::ScalarType::INT:                                  \
				function<int>( __VA_ARGS__ );                               \
				break;                                                      \
			case iAITKIO::ScalarType::UINT:                                 \
				function<unsigned int>( __VA_ARGS__ );                      \
				break;                                                      \
			case iAITKIO::ScalarType::LONG:                                 \
				function<long>( __VA_ARGS__ );                              \
				break;                                                      \
			case iAITKIO::ScalarType::ULONG:                                \
				function<unsigned long>( __VA_ARGS__ );                     \
				break;                                                      \
			case iAITKIO::ScalarType::LONGLONG:                             \
				function<long long>( __VA_ARGS__ );                         \
				break;                                                      \
			case iAITKIO::ScalarType::ULONGLONG:                            \
				function<unsigned long long>( __VA_ARGS__ );                \
				break;                                                      \
			case iAITKIO::ScalarType::FLOAT:                                \
				function<float>( __VA_ARGS__ );                             \
				break;                                                      \
			case iAITKIO::ScalarType::DOUBLE:                               \
				function<double>( __VA_ARGS__ );                            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString("Typed Call: Unknown scalar pixel type (%1).")  \
					.arg(static_cast<int>(itk_scalar_type))                 \
					.toStdString().c_str() );                               \
		}                                                                   \
	}                                                                       \
	else if (itk_pixel_type == iAITKIO::PixelType::RGB)                     \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case iAITKIO::ScalarType::UCHAR:                                \
				function<itk::RGBPixel< unsigned char >>( __VA_ARGS__ );    \
				break;                                                      \
			case iAITKIO::ScalarType::CHAR:                                 \
				function<itk::RGBPixel<char>>( __VA_ARGS__ );               \
				break;                                                      \
			case iAITKIO::ScalarType::SHORT:                                \
				function<itk::RGBPixel<short>>( __VA_ARGS__ );              \
				break;                                                      \
			case iAITKIO::ScalarType::USHORT:                               \
				function<itk::RGBPixel<unsigned short>>( __VA_ARGS__ );     \
				break;                                                      \
			case iAITKIO::ScalarType::INT:                                  \
				function<itk::RGBPixel<int>>( __VA_ARGS__ );                \
				break;                                                      \
			case iAITKIO::ScalarType::UINT:                                 \
				function<itk::RGBPixel<unsigned int>>( __VA_ARGS__ );       \
				break;                                                      \
			case iAITKIO::ScalarType::LONG:                                 \
				function<itk::RGBPixel<long>>( __VA_ARGS__ );               \
				break;                                                      \
			case iAITKIO::ScalarType::ULONG:                                \
				function<itk::RGBPixel<unsigned long>>( __VA_ARGS__ );      \
				break;                                                      \
			case iAITKIO::ScalarType::LONGLONG:                             \
				function<itk::RGBPixel<long long>>( __VA_ARGS__ );          \
				break;                                                      \
			case iAITKIO::ScalarType::ULONGLONG:                            \
				function<itk::RGBPixel<unsigned long long>>( __VA_ARGS__ ); \
				break;                                                      \
			case iAITKIO::ScalarType::FLOAT:                                \
				function<itk::RGBPixel<float>>( __VA_ARGS__ );              \
				break;                                                      \
			case iAITKIO::ScalarType::DOUBLE:                               \
				function<itk::RGBPixel<double>>( __VA_ARGS__ );             \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString("Typed Call: Unknown scalar pixel type (%1).")  \
					.arg(static_cast<int>(itk_scalar_type))                 \
					.toLatin1().data() );                                   \
		}                                                                   \
	}                                                                       \
	else if ( itk_pixel_type == iAITKIO::PixelType::RGBA ||                 \
		 itk_pixel_type == iAITKIO::PixelType::VECTOR )                     \
	{                                                                       \
		switch ( itk_scalar_type )                                          \
		{                                                                   \
			case iAITKIO::ScalarType::UCHAR:                                \
				function<itk::RGBAPixel< unsigned char >>( __VA_ARGS__ );   \
				break;                                                      \
			case iAITKIO::ScalarType::CHAR:                                 \
				function<itk::RGBAPixel<char>>( __VA_ARGS__ );              \
				break;                                                      \
			case iAITKIO::ScalarType::SHORT:                                \
				function<itk::RGBAPixel<short>>( __VA_ARGS__ );             \
				break;                                                      \
			case iAITKIO::ScalarType::USHORT:                               \
				function<itk::RGBAPixel<unsigned short>>( __VA_ARGS__ );    \
				break;                                                      \
			case iAITKIO::ScalarType::INT:                                  \
				function<itk::RGBAPixel<int>>( __VA_ARGS__ );               \
				break;                                                      \
			case iAITKIO::ScalarType::UINT:                                 \
				function<itk::RGBAPixel<unsigned int>>( __VA_ARGS__ );      \
				break;                                                      \
			case iAITKIO::ScalarType::LONG:                                 \
				function<itk::RGBAPixel<long>>( __VA_ARGS__ );              \
				break;                                                      \
			case iAITKIO::ScalarType::ULONG:                                \
				function<itk::RGBAPixel<unsigned long>>( __VA_ARGS__ );     \
				break;                                                      \
			case iAITKIO::ScalarType::LONGLONG:                             \
				function<itk::RGBAPixel<long long>>( __VA_ARGS__ );         \
				break;                                                      \
			case iAITKIO::ScalarType::ULONGLONG:                            \
				function<itk::RGBAPixel<unsigned long long>>( __VA_ARGS__ );\
				break;                                                      \
			case iAITKIO::ScalarType::FLOAT:                                \
				function<itk::RGBAPixel<float>>( __VA_ARGS__ );             \
				break;                                                      \
			case iAITKIO::ScalarType::DOUBLE:                               \
				function<itk::RGBAPixel<double>>( __VA_ARGS__ );            \
				break;                                                      \
			default:                                                        \
				throw itk::ExceptionObject( __FILE__, __LINE__,             \
					QString("Typed Call: Unknown scalar pixel type (%1).")  \
					.arg(static_cast<int>(itk_scalar_type))                 \
					.toLatin1().data() );                                   \
		}                                                                   \
	}                                                                       \
	else                                                                    \
	{                                                                       \
		throw itk::ExceptionObject(__FILE__, __LINE__,                      \
			QString("Typed Call: Unknown pixel type (%1).")                 \
				.arg(static_cast<int>(itk_pixel_type)).toLatin1().data() ); \
	}                                                                       \
}

#define VTK_EXTENDED_TYPED_CALL(function, vtk_scalar_type, number_of_components,...)     \
{                                                                                        \
	if ( number_of_components < 2 )                                                      \
	{                                                                                    \
		switch ( vtk_scalar_type )                                                       \
		{                                                                                \
			case VTK_UNSIGNED_CHAR:                                                      \
				function<unsigned char>( __VA_ARGS__ );                                  \
				break;                                                                   \
			case VTK_SIGNED_CHAR:                                                        \
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
			case VTK_SIGNED_CHAR:                                                        \
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

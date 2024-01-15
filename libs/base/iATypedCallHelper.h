// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdexcept>

//! Call a specific instance of a templated function based on ITK's scalar type
//! Requirements:
//! |function| return type must be void
//!
//! Usage:
//!
//! having for instance such |type| variable:
//!   ColumnType type = INT;
//! and such |foo| function definition:
//!   template <ColumnType T>
//!   void foo(t1 arg1, t2 arg2) {
//!   }
//!
//! instead of writing (won't compile):
//!   foo<type>(arg1, arg2);
//! write this:
//!   xxx_TYPED_CALL(foo, type, arg1, arg2);
//!
//!
//! for foo with 0 arguments write this:
//!   xxx_TYPED_CALL(foo, type, );
#define ITK_TYPED_CALL(function, itk_scalar_type, ...)      \
{                                                           \
	switch (itk_scalar_type)                                \
	{                                                       \
	case iAITKIO::ScalarType::UCHAR:                           \
		function<unsigned char>(__VA_ARGS__);               \
		break;                                              \
	case iAITKIO::ScalarType::CHAR:                            \
		function<char>(__VA_ARGS__);                        \
		break;                                              \
	case iAITKIO::ScalarType::SHORT:                           \
		function<short>(__VA_ARGS__);                       \
		break;                                              \
	case iAITKIO::ScalarType::USHORT:                          \
		function<unsigned short>(__VA_ARGS__);              \
		break;                                              \
	case iAITKIO::ScalarType::INT:                             \
		function<int>(__VA_ARGS__);                         \
		break;                                              \
	case iAITKIO::ScalarType::UINT:                            \
		function<unsigned int>(__VA_ARGS__);                \
		break;                                              \
	case iAITKIO::ScalarType::LONG:                            \
		function<long>(__VA_ARGS__);                        \
		break;                                              \
	case iAITKIO::ScalarType::ULONG:                           \
		function<unsigned long>(__VA_ARGS__);               \
		break;                                              \
	case iAITKIO::ScalarType::LONGLONG:                        \
		function<long long>(__VA_ARGS__);                   \
		break;                                              \
	case iAITKIO::ScalarType::ULONGLONG:                       \
		function<unsigned long long>(__VA_ARGS__);          \
		break;                                              \
	case iAITKIO::ScalarType::FLOAT:                           \
		function<float>(__VA_ARGS__);                       \
		break;                                              \
	case iAITKIO::ScalarType::DOUBLE:                          \
		function<double>(__VA_ARGS__);                      \
		break;                                              \
	default:                                                \
		throw itk::ExceptionObject(__FILE__, __LINE__,      \
			QString("Typed Call: Unknown component type (%1).") \
				.arg(static_cast<unsigned char>(itk_scalar_type)) \
				.toStdString().c_str());                    \
		break;                                              \
	}                                                       \
}

//! Call a specific instance of a templated function based on VTK's scalar type
#define VTK_TYPED_CALL(function, vtk_scalar_type, ...)      \
{                                                           \
	switch (vtk_scalar_type)                                \
	{                                                       \
	case VTK_UNSIGNED_CHAR:                                 \
		function<unsigned char>(__VA_ARGS__);               \
		break;                                              \
	case VTK_SIGNED_CHAR:                                   \
	case VTK_CHAR:                                          \
		function<char>(__VA_ARGS__);                        \
		break;                                              \
	case VTK_SHORT:                                         \
		function<short>(__VA_ARGS__);                       \
		break;                                              \
	case VTK_UNSIGNED_SHORT:                                \
		function<unsigned short>(__VA_ARGS__);              \
		break;                                              \
	case VTK_INT:                                           \
		function<int>(__VA_ARGS__);                         \
		break;                                              \
	case VTK_UNSIGNED_INT:                                  \
		function<unsigned int>(__VA_ARGS__);                \
		break;                                              \
	case VTK_LONG:                                          \
		function<long>(__VA_ARGS__);                        \
		break;                                              \
	case VTK_UNSIGNED_LONG:                                 \
		function<unsigned long>(__VA_ARGS__);               \
		break;                                              \
	case VTK_LONG_LONG:                                     \
		function<long long>(__VA_ARGS__);                   \
		break;                                              \
	case VTK_UNSIGNED_LONG_LONG:                            \
		function<unsigned long long>(__VA_ARGS__);          \
		break;                                              \
	case VTK_FLOAT:                                         \
		function<float>(__VA_ARGS__);                       \
		break;                                              \
	case VTK_DOUBLE:                                        \
		function<double>(__VA_ARGS__);                      \
		break;                                              \
	default:                                                \
	throw std::runtime_error(QString(                       \
		"Typed Call: Unknown component type (%1).").arg(vtk_scalar_type).toStdString()); \
	break;                                                  \
	}                                                       \
}

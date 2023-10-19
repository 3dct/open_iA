// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAItkVersion.h"

#if ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5, 1, 0)
#include <itkCommonEnums.h>
#else
#include <itkImageIOBase.h>
#endif

namespace iAITKIO
{
	static const int Dim = 3;    //! merge with defines -> DIM
#if ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5, 1, 0)
	using PixelType = itk::CommonEnums::IOPixel;
	using ScalarType = itk::CommonEnums::IOComponent;
#else
	using PixelType = itk::ImageIOBase::IOPixelType;
	using ScalarType = itk::ImageIOBase::IOComponentType;
#endif
}
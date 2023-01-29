// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include "iAItkVersion.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>

class iAProgress;

class QString;

namespace iAITKIO
{
	static const int Dim = 3;
	using ImageBaseType = itk::ImageBase<Dim>;
	using ImagePointer = ImageBaseType::Pointer;
	using ImagePtr = ImageBaseType*;
#if ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5, 1, 0)
	using PixelType = itk::CommonEnums::IOPixel;
	using ScalarType = itk::CommonEnums::IOComponent;
#else
	using PixelType = itk::ImageIOBase::IOPixelType;
	using ScalarType = itk::ImageIOBase::IOComponentType;
#endif

	// TODO:
	//     - check usage - replace with iAFileTypeRegistry::createIO where it makes sense to support broader range of file types
	//     - check iAToolsITK
	iAbase_API ImagePointer readFile(QString const& fileName, PixelType& pixelType, ScalarType& scalarType, bool releaseFlag, iAProgress const * progress = nullptr);
	iAbase_API void writeFile(QString const& fileName, ImagePtr image, ScalarType scalarType, bool useCompression = false, iAProgress const * progress = nullptr);
} // namespace iAITKIO

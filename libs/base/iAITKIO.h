// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include "iAitkTypes.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include <itkImageBase.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

class iAProgress;

class QString;

//! Support for loading and storing images via ITK's file I/O
namespace iAITKIO
{
	using ImageBaseType = itk::ImageBase<Dim>;
	using ImagePointer = ImageBaseType::Pointer;
	using ImagePtr = ImageBaseType*;

	// TODO:
	//     - check usage - replace with iAFileTypeRegistry::createIO where it makes sense to support broader range of file types
	//     - check iAToolsITK
	//! Use ITK's file IO to read files from a given file name, returning its pixel and scalar type
	//! @param[in] fileName the name of the file to read
	//! @param[out] pixelType set to the pixel type of the file that has been read
	//! @param[out] scalarType set to the scalar type of pixel values in the file that has been read
	//! @param[in] releaseFlag whether or not to set the release flag on the file reader
	//! @param[in] progress optional progress reporting capabilities via iAProgress
	iAbase_API ImagePointer readFile(QString const& fileName, PixelType& pixelType, ScalarType& scalarType, bool releaseFlag, iAProgress const * progress = nullptr);
	//! Use ITK's file IO to write an image to a file
	//! @param[in] fileName the name of the file to write
	//! @param[in] image the image to write
	//! @param[in] scalarType the scalar type of pixel values in the file to write
	//! @param[in] useCompression whether the file should be compressed (if the file type specified via extension in fileName parameter supports it)
	//! @param[in] progress optional progress reporting capabilities via iAProgress
	iAbase_API void writeFile(QString const& fileName, ImagePtr image, ScalarType scalarType, bool useCompression = false, iAProgress const * progress = nullptr);
} // namespace iAITKIO

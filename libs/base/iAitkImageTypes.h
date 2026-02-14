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

//! Basic image type definition (type-agnostic) 
namespace iAITKIO
{
	using ImageBaseType = itk::ImageBase<Dim>;
	using ImagePointer = ImageBaseType::Pointer;
	using ImagePtr = ImageBaseType*;
}


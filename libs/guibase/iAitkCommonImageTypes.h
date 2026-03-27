// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImage.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// TODO: Replace with some other definition / template?
const int Dimensions = 3;
using LabelPixelType = int;
using LabelImageType = itk::Image<LabelPixelType, Dimensions>;
using LabelImagePointer = LabelImageType::Pointer;

using ProbabilityImageType = itk::Image<double, 3>;
using ProbabilityImagePointer = ProbabilityImageType::Pointer;

using IntImage = itk::Image<int, Dimensions>;
using IntImagePointer = IntImage::Pointer;

using DoubleImage = itk::Image<double, Dimensions>;
using DoubleImagePointer = DoubleImage::Pointer;

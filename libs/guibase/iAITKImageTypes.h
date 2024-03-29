// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkImage.h>

// TODO: Replace with some other definition / template?
const int Dimensions = 3;
typedef int LabelPixelType;
typedef itk::Image<LabelPixelType, Dimensions> LabelImageType;
typedef LabelImageType::Pointer LabelImagePointer;

typedef double ProbabilityPixelType;
typedef itk::Image<ProbabilityPixelType, 3> ProbabilityImageType;
typedef ProbabilityImageType::Pointer ProbabilityImagePointer;

typedef itk::Image<int, Dimensions> IntImage;
typedef IntImage::Pointer IntImagePointer;

typedef itk::Image<double, Dimensions> DoubleImage;
typedef DoubleImage::Pointer DoubleImagePointer;

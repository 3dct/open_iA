// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAITKImageTypes.h>

#include <vtkSmartPointer.h>

class vtkImageData;

/*
typedef itk::Image<double, 3> PriorModelImageType;
typedef itk::SmartPointer<PriorModelImageType> PriorModelImagePointer;
*/
typedef vtkSmartPointer<vtkImageData> PriorModelImagePointer;

//typedef int PreviewPixelType;
//typedef itk::Image<PreviewPixelType, Dimensions> PreviewImageType;
//typedef PreviewImageType::Pointer PreviewImagePointer;

//typedef iAITKIO::ImagePointer ClusterImageType;

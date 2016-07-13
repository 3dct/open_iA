/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include <itkImage.h>
#include "iAITKIO.h" // TODO: replace?

#include <vtkSmartPointer.h>

class vtkImageData;

// TODO: data type -> short/char
typedef int LabelPixelType;
typedef itk::Image<LabelPixelType, 3> LabelImageType;
typedef LabelImageType::Pointer LabelImagePointer;

typedef itk::Image<double, 3> ProbabilityImageType;
typedef ProbabilityImageType::Pointer ProbabilityImagePointer;

/*
typedef itk::Image<double, 3> PriorModelImageType;
typedef itk::SmartPointer<PriorModelImageType> PriorModelImagePointer;
*/
typedef vtkSmartPointer<vtkImageData> PriorModelImagePointer;

typedef int PreviewPixelType;
typedef itk::Image<PreviewPixelType, 3> PreviewImageType;
typedef PreviewImageType::Pointer PreviewImagePointer;

typedef iAITKIO::ImagePointer ClusterImageType;

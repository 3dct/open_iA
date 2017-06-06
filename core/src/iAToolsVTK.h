/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

class vtkImageData;

class QString;

// image creation:
void DeepCopy(vtkSmartPointer<vtkImageData> input, vtkSmartPointer<vtkImageData> output);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(vtkSmartPointer<vtkImageData> img);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int dimensions[3], double spacing[3]);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int dimensions[3], double spacing[3], int numComponents);

// image I/O (using ITK methods of iAITKIO)
void StoreImage(vtkSmartPointer<vtkImageData> image, QString const & filename, bool useCompression = true);
vtkSmartPointer<vtkImageData> ReadImage(QString const & filename, bool releaseFlag);

void WriteSingleSliceImage(QString const & filename, vtkImageData* imageData);

bool isVtkIntegerType(int type);

#define FOR_VTKIMG_PIXELS(img, x, y, z) \
    for (int x = 0; x < img->GetDimensions()[0]; ++x) \
        for (int y = 0; y < img->GetDimensions()[1]; ++y) \
            for (int z = 0; z < img->GetDimensions()[2]; ++z)

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QMap>

class vtkImageData;

class QString;
class QStringList;

// image creation:
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(vtkSmartPointer<vtkImageData> img);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int const dimensions[3], double const spacing[3]);
open_iA_Core_API vtkSmartPointer<vtkImageData> AllocateImage(int vtkType, int const dimensions[3], double const spacing[3], int numComponents);

// image I/O (using ITK methods of iAITKIO)
open_iA_Core_API void StoreImage(vtkSmartPointer<vtkImageData> image, QString const & filename, bool useCompression = true);
open_iA_Core_API vtkSmartPointer<vtkImageData> ReadImage(QString const & filename, bool releaseFlag);

open_iA_Core_API void WriteSingleSliceImage(QString const & filename, vtkImageData* imageData);

open_iA_Core_API int MapVTKTypeStringToInt(QString const & vtkTypeName);

open_iA_Core_API int MapVTKTypeStringToSize(QString const & vtkTypeString);

open_iA_Core_API vtkSmartPointer<vtkImageData> CastVTKImage(vtkSmartPointer<vtkImageData> img, int DestType);

open_iA_Core_API bool isVtkIntegerType(int type);

open_iA_Core_API QStringList const & VTKDataTypeList();

open_iA_Core_API QMap<int, QString> const & RenderModeMap();

open_iA_Core_API int MapRenderModeToEnum(QString const &);

#define FOR_VTKIMG_PIXELS(img, x, y, z) \
for (int z = 0; z < img->GetDimensions()[2]; ++z) \
	for (int y = 0; y < img->GetDimensions()[1]; ++y) \
		for (int x = 0; x < img->GetDimensions()[0]; ++x)

#define FOR_VTKIMG_PIXELS_IDX(img, idx) \
for (size_t idx = 0; idx < img->GetDimensions()[0]*img->GetDimensions()[1]*img->GetDimensions()[2]; ++idx)

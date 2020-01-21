/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

class vtkCamera;
class vtkImageData;

class QString;
class QStringList;

//! Create a VTK image that has the same properties (type, size, spacing) as the given image.
//! @param img image whose type, size and spacing will be used to create the result image;
//!        its data will not be copied over to the new image
//! @return an image that has the same type, size and spacing as the image given as parameter,
//!         with allocated but uninitialized pixel data.
open_iA_Core_API vtkSmartPointer<vtkImageData> allocateImage(vtkSmartPointer<vtkImageData> img);

//! Create a VTK image with the given properties.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...) for the voxel data type to use in the new image
//! @param dimensions the size of the image in the 3 dimensions.
//! @param spacing the spacing (distance of voxels) in each of the 3 dimension directions.
//! @return an image with a single component and the type, size and spacing as the given parameters,
//!         with allocated but uninitialized pixel data.
open_iA_Core_API vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3]);

//! Create a VTK image with the given properties.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...) for the voxel data type to use in the new image
//! @param dimensions the size of the image in the 3 dimensions.
//! @param spacing the spacing (distance of voxels) in each of the 3 dimension directions.
//! @param numComponents the number of components in each voxel.
//! @return an image with the type, size, spacing and number of components as the given parameters,
//!         with allocated but uninitialized pixel data.
open_iA_Core_API vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3], int numComponents);

//! Stores an image on disk (typically in .mhd format).
//! @param image the image to store
//! @param filename the name of the file to write to.
//! @param useCompression whether the file should be compressed (.zraw) or not (.raw) in case we are storing .mhd files
open_iA_Core_API void storeImage(vtkSmartPointer<vtkImageData> image, QString const & filename, bool useCompression = true);

//! Read an image from disk into a VTK image.
//! @param filename the name of the file to read.
//! @param releaseFlag whether the ITK release flag should be set on the ITK image reader
//! @return the vtk image as read from the given file
open_iA_Core_API vtkSmartPointer<vtkImageData> readImage(QString const & filename, bool releaseFlag);

//! Write a single slice image to a common 2D picture format
//! @param filename the name of the file to write to; this is expected to have an extension of tif, png, jpg or bmp;
//!        the type of the file written will be chosen according to this extension
//! @param imageData the image to write; this already needs to be a 2D image (i.e., size in Z dimension = 1)
open_iA_Core_API void writeSingleSliceImage(QString const & filename, vtkImageData* imageData);

//! Returns the size (in bytes) of the given VTK type.
//! @param vtkType a VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the size in bytes of the given type (VTK_CHAR -> 1, ...),
//!        or 0 if it's an unknown type
open_iA_Core_API size_t mapVTKTypeToSize(int vtkType);

//! Cast the given VTK image to another data type.
//! @param img the input image
//! @param dstType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...) to cast to
//! @return a VTK image of the specified type
open_iA_Core_API vtkSmartPointer<vtkImageData> castVTKImage(vtkSmartPointer<vtkImageData> img, int dstType);

//! Check whether the given type is integer.
//! @param type a VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return true if the given VTK type holds integer numbers,
//!        false if it holds floating point numbers
open_iA_Core_API bool isVtkIntegerType(int type);

//! Returns a human-readable list of available data types for a single pixel/voxel.
//! @param withLongLongTypes
open_iA_Core_API QStringList const & readableDataTypeList(bool withLongLongTypes);

//! Maps a given data type string to the corresponding VTK type identifier.
//! @param dataTypeName an entry from the list of readable data types
//!        (see readableDataTypeList())
//! @return the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//!        for the given readable data type name, or -1 if the given name
//!        is not on the list
open_iA_Core_API int mapReadableDataTypeToVTKType(QString const & dataTypeName);

//! Maps a given VTK type to the corresponding readable data type.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the name (in the list as returned by readableDataTypeList())
//!        of the given VTK type identifier,
//!        or -1 if the given VTK type identifier is unknown
open_iA_Core_API QString mapVTKTypeToReadableDataType(int vtkType);


open_iA_Core_API QMap<int, QString> const & RenderModeMap();

open_iA_Core_API int mapRenderModeToEnum(QString const &);

#define FOR_VTKIMG_PIXELS(img, x, y, z) \
for (int z = 0; z < img->GetDimensions()[2]; ++z) \
	for (int y = 0; y < img->GetDimensions()[1]; ++y) \
		for (int x = 0; x < img->GetDimensions()[0]; ++x)

#define FOR_VTKIMG_PIXELS_IDX(img, idx) \
for (size_t idx = 0; idx < img->GetDimensions()[0]*img->GetDimensions()[1]*img->GetDimensions()[2]; ++idx)


enum iACameraPosition
{
	PX,
	MX,
	PY,
	MY,
	PZ,
	MZ,
	Iso
};

open_iA_Core_API void setCamPosition(vtkCamera* cam, iACameraPosition mode);

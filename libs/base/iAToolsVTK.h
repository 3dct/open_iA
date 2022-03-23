/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAbase_export.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QStringList>

class iAProgress;

class vtkCamera;
class vtkImageData;

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkScalarsToColors;

class QString;

//! Create a VTK image that has the same properties (type, size, spacing) as the given image.
//! @param img image whose type, size and spacing will be used to create the result image;
//!        its data will not be copied over to the new image
//! @return an image that has the same type, size and spacing as the image given as parameter,
//!         with allocated but uninitialized pixel data.
iAbase_API vtkSmartPointer<vtkImageData> allocateImage(vtkSmartPointer<vtkImageData> img);

//! Create a VTK image with the given properties.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...) for the voxel data type to use in the new image
//! @param dimensions the size of the image in the 3 dimensions.
//! @param spacing the spacing (distance of voxels) in each of the 3 dimension directions.
//! @return an image with a single component and the type, size and spacing as the given parameters,
//!         with allocated but uninitialized pixel data.
iAbase_API vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3]);

//! Create a VTK image with the given properties.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...) for the voxel data type to use in the new image
//! @param dimensions the size of the image in the 3 dimensions.
//! @param spacing the spacing (distance of voxels) in each of the 3 dimension directions.
//! @param numComponents the number of components in each voxel.
//! @return an image with the type, size, spacing and number of components as the given parameters,
//!         with allocated but uninitialized pixel data.
iAbase_API vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3], int numComponents);

//! fill all pixels in the given image with the given value
//! @param img image to be filled
//! @param value used to fill each voxel in given image
//! @param p if given, used to report progress
iAbase_API void fillImage(vtkSmartPointer<vtkImageData> img, double const value, iAProgress* p = nullptr);

//! multiply all values of an image with the given value
//! @param imgDst the image to be multiplied
//! @param value multiplier used for each voxel
//! @param p if given, used to report progress
iAbase_API void multiplyImage(vtkSmartPointer<vtkImageData> imgDst, double value, iAProgress* p = nullptr);

//! add values of one image to the values of another image
//! @param imgDst destination image (will be modified)
//! @param imgToAdd image with values to be added to imgDst (will not be modified)
//! @param p if given, used to report progress
iAbase_API void addImages(vtkSmartPointer<vtkImageData> imgDst, vtkSmartPointer<vtkImageData> const imgToAdd, iAProgress* p = nullptr);

//! Stores an image on disk (typically in .mhd format).
//! @param img the image to store
//! @param filename the name of the file to write to.
//! @param useCompression whether the file should be compressed (.zraw) or not (.raw) in case we are storing .mhd files
iAbase_API void storeImage(vtkSmartPointer<vtkImageData> img, QString const & filename, bool useCompression = true);

//! Read an image from disk into a VTK image.
//! @param filename the name of the file to read.
//! @param releaseFlag whether the ITK release flag should be set on the ITK image reader
//! @param ptr the smart pointer in which to store the loaded image (it will be assigned a new vtkImageData)
iAbase_API void readImage(QString const& filename, bool releaseFlag, vtkSmartPointer<vtkImageData>& ptr);

//! Write a single slice image to a common 2D picture format
//! @param filename the name of the file to write to; this is expected to have an extension of tif, png, jpg or bmp;
//!        the type of the file written will be chosen according to this extension
//! @param img the image to write; this already needs to be a 2D image (i.e., size in Z dimension = 1)
iAbase_API void writeSingleSliceImage(QString const & filename, vtkImageData* img);

//! Returns the size (in bytes) of the given VTK type.
//! @param vtkType a VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the size in bytes of the given type (VTK_CHAR -> 1, ...),
//!        or 0 if it's an unknown type
iAbase_API size_t mapVTKTypeToSize(int vtkType);

//! Check whether the given image holds integer numbers.
//! @param img a VTK image
//! @return true if the values in the given VTK image are integer numbers,
//!        false if it holds floating point numbers
iAbase_API bool isVtkIntegerImage(vtkImageData* img);

//! Returns a human-readable list of available data types for a single pixel/voxel.
//! @param withLongLongTypes
iAbase_API QStringList const & readableDataTypeList(bool withLongLongTypes);

//! Maps a given data type string to the corresponding VTK type identifier.
//! @param dataTypeName an entry from the list of readable data types
//!        (see readableDataTypeList())
//! @return the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//!        for the given readable data type name, or -1 if the given name
//!        is not on the list
iAbase_API int mapReadableDataTypeToVTKType(QString const & dataTypeName);

//! Maps a given VTK type to the corresponding readable data type.
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the name (in the list as returned by readableDataTypeList())
//!        of the given VTK type identifier,
//!        or -1 if the given VTK type identifier is unknown
iAbase_API QString mapVTKTypeToReadableDataType(int vtkType);


iAbase_API QMap<int, QString> const & RenderModeMap();

iAbase_API int mapRenderModeToEnum(QString const &);

#define FOR_VTKIMG_PIXELS(img, x, y, z) \
for (int z = 0; z < img->GetDimensions()[2]; ++z) \
	for (int y = 0; y < img->GetDimensions()[1]; ++y) \
		for (int x = 0; x < img->GetDimensions()[0]; ++x)

#define FOR_VTKIMG_PIXELS_IDX(img, idx) \
for (size_t idx = 0; idx < img->GetDimensions()[0]*img->GetDimensions()[1]*img->GetDimensions()[2]; ++idx)

// analog to above macros to iterate over every pixel in a templated way with img->GetScalarPointer?

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

iAbase_API void setCamPosition(vtkCamera* cam, iACameraPosition mode);

// maybe better in iALUT (but that is in charts library -> move that to separate colors lib? to base?)
iAbase_API void convertLUTToTF(vtkSmartPointer<vtkLookupTable> src, vtkSmartPointer<vtkColorTransferFunction> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, double alphaOverride = -1);
iAbase_API void convertTFToLUT(vtkSmartPointer<vtkLookupTable> dst, vtkSmartPointer<vtkScalarsToColors> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, int numCols, double const* lutRange = nullptr, bool reverse = false);
// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include "iAVec3.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QStringList>
#include <QVariant>    // for QVariantMap (at least under Qt 5.15.2)

class iAProgress;

class vtkCamera;
class vtkImageData;

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkScalarsToColors;

class QString;


//  ----- Image helpers: simplify allocate, fill, add, multiply, read, store, ... -----

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
//! @param progress an optional progress link; if != null, the file writer will trigger its progress signal
iAbase_API void storeImage(vtkSmartPointer<vtkImageData> img, QString const & filename, bool useCompression = true, iAProgress const* progress = nullptr);

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

//! Check whether the given image holds integer numbers.
//! @param img a VTK image
//! @return true if the values in the given VTK image are integer numbers,
//!        false if it holds floating point numbers
iAbase_API bool isVtkIntegerImage(vtkImageData* img);

//! Given index and size parameters in a QVariantMap, adjust these parameters
//! so that they specify a region of interest that lies completely within the given image
//! @param params map of parameters (which should contain values for "Index" and "Size", as iAValueType::Vector3i, i.e. QVector<int>)
//! @param img the image whose size determines the clamping of parameters
iAbase_API void adjustIndexAndSizeToImage(QVariantMap& params, vtkImageData* img);

//! Checks whether an image is flat.
//! @param img a VTK image
//! @return whether the image is flat in any direction, i.e. whether one side is only 1 pixel wide.
iAbase_API bool isFlat(vtkImageData* img);


// ----- Coordinate conversions (world <-> image/voxel coordinates) -----

//! Translate from world coordinates to voxel coordinates for the given image
//! @param img a VTK image
//! @param worldCoord world (=scene) coordinates (3 components: x, y, z)
//! @param voxelCoord place for storing 3 components of voxel coordinates in img for the given world coordinates (clamped)
iAbase_API void mapWorldToVoxelCoords(vtkImageData* img, double const* worldCoord, double* voxelCoord);
//! Translate from world coordinates to voxel indices for the given image
//! @param img a VTK image
//! @param worldCoord world (=scene) coordinates (3 components: x, y, z)
//! @return voxel indices, the coordinates in img for the given world coordinates (clamped)
iAbase_API iAVec3i mapWorldCoordsToIndex(vtkImageData* img, double const* worldCoord);


// ----- Data types -----

//! Returns the size (in bytes) of the given VTK type.
//! @param vtkType a VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the size in bytes of the given type (VTK_SIGNED_CHAR/VTK_UNSIGNED_CHAR -> 1, ...),
//!        or 0 if it's an unknown type
iAbase_API size_t mapVTKTypeToSize(int vtkType);
//! Returns a human-readable list of available data types for a single pixel/voxel.
//! @param withLongLongTypes
iAbase_API QStringList const & readableDataTypeList(bool withLongLongTypes);
//! Maps a given data type string to the corresponding VTK type identifier.
//! Reverse of mapVTKTypeToReadableDataType
//! @param dataTypeName an entry from the list of readable data types
//!        (see readableDataTypeList())
//! @return the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//!        for the given readable data type name, or -1 if the given name
//!        is not on the list
iAbase_API int mapReadableDataTypeToVTKType(QString const & dataTypeName);
//! Maps a given VTK type to the corresponding readable data type.
//! Reverse of mapReadableDataTypeToVTKType
//! @param vtkType the VTK type identifier (VTK_INT, VTK_UNSIGNED_CHAR, ...)
//! @return the name (in the list as returned by readableDataTypeList())
//!        of the given VTK type identifier,
//!        or -1 if the given VTK type identifier is unknown
iAbase_API QString mapVTKTypeToReadableDataType(int vtkType);


// Byte Order handling - is a class mainly for simplifying exporting the two string constants
class iAbase_API ByteOrder
{
public:
	static const QString BigEndianStr;
	static const QString LittleEndianStr;
	//! Returns a human-readable list of available byte orders (little/big endian)
	static QStringList const& stringList();
	//! Maps a given readable string to the according VTK byte order type.
	//! Reverse to mapVTKByteOrderToReadable
	static int mapStringToVTKType(QString const& name);
	//! Maps a given VTK byte order type to the according readable string.
	//! Reverse to mapReadableByteOrderToVTKType
	static QString mapVTKTypeToString(int byteOrder);
};


// ----- Render Modes -----

//! a map of available render modes in vtkSmartVolumeMapper to their names
iAbase_API QMap<int, QString> const & RenderModeMap();
//! map the given render mode name to the respective enum in the render mode map
iAbase_API int mapRenderModeToEnum(QString const &);


// ----- Image Iteration -----

#define FOR_VTKIMG_PIXELS(img, x, y, z) \
for (int z = 0; z < img->GetDimensions()[2]; ++z) \
	for (int y = 0; y < img->GetDimensions()[1]; ++y) \
		for (int x = 0; x < img->GetDimensions()[0]; ++x)

#define FOR_VTKIMG_PIXELS_IDX(img, idx) \
for (size_t idx = 0; idx < img->GetDimensions()[0]*img->GetDimensions()[1]*img->GetDimensions()[2]; ++idx)

// ToDo: analog to above macros to iterate over every pixel in a templated way with img->GetScalarPointer?


// ----- Camera -----

//! Predefined camera positions; view along every positive (P) and negative (M) axis (x, y, z),
//! as well as an isometric perspective
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
//! Set given camera position to one of the predefined positions available in iACameraPosition
iAbase_API void setCamPosition(vtkCamera* cam, iACameraPosition mode);

iAbase_API void copyCameraParams(vtkCamera* dst, vtkCamera* src);


// ----- Lookup table / Color transfer function conversion -----

// maybe better in iALUT (but that is in charts library -> move that to separate colors lib? to base?)
iAbase_API void convertLUTToTF(vtkSmartPointer<vtkLookupTable> src, vtkSmartPointer<vtkColorTransferFunction> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, double alphaOverride = -1);
iAbase_API void convertTFToLUT(vtkSmartPointer<vtkLookupTable> dst, vtkSmartPointer<vtkScalarsToColors> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, int numCols, double const* lutRange = nullptr, bool reverse = false);

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
#include "iAToolsVTK.h"

#include "iAConnector.h"
#include "iAFileUtils.h"
#include "iAITKIO.h"
#include "iALog.h"
#include "iAMathUtility.h"      // for mapValue
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include "iAValueTypeVectorHelpers.h"        // for variantVectorFrom
#include "iAVtkDraw.h"
#include "iAVtkVersion.h"

#include <vtkBMPWriter.h>
#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkImageWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkObjectFactory.h>
#include <vtkPNGWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkSmartVolumeMapper.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageReader.h>    // for VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN, ...
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

//#include <omp.h>    // for  omp_get_thread_num

// declared in iAVtkDraw.h
vtkStandardNewMacro(iAvtkImageData);


vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3], int numComponents)
{
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();
	result->SetDimensions(dimensions);
	result->AllocateScalars(vtkType, numComponents);
	double nonConstSpc[3];
	std::copy(spacing, spacing + 3, nonConstSpc);
	result->SetSpacing(nonConstSpc);
	return result;
}

vtkSmartPointer<vtkImageData> allocateImage(int vtkType, int const dimensions[3], double const spacing[3])
{
	return allocateImage(vtkType, dimensions, spacing, 1);
}

vtkSmartPointer<vtkImageData> allocateImage(vtkSmartPointer<vtkImageData> img)
{
	return allocateImage(img->GetScalarType(), img->GetDimensions(), img->GetSpacing());
}

namespace
{
	template <typename T>
	void processImg_tmpl(void* voidPtr, std::function<double(double)> func, qint64 count, iAProgress* p)
	{
		T* ptr = static_cast<T*>(voidPtr);
		qint64 step = count / 100;
		qint64 lastEmit = 0;
//#pragma omp parallel for		// no (significant) speedup
		for (qint64 i = 0; i < count; ++i)
		{
			ptr[i] = func(ptr[i]);
			if (p && i > (lastEmit + step)) // && omp_get_thread_num() == 0)	// in case OpenMP used, see https://stackoverflow.com/questions/28050669/can-i-report-progress-for-openmp-tasks
			{
				p->emitProgress(100 * i / count);
				lastEmit = i;
			}
		}
		if (p)
		{
			p->emitProgress(100);
		}
	}

	void processImg(vtkSmartPointer<vtkImageData> img, std::function<double(double)> func, iAProgress* p)
	{
		int const* dim = img->GetDimensions();
		qint64 count = static_cast<qint64>(dim[0]) * static_cast<qint64>(dim[1]) * dim[2];
		void* voidPtr = img->GetScalarPointer();
		VTK_TYPED_CALL(processImg_tmpl, img->GetScalarType(), voidPtr, func, count, p);
	}

	template <typename T, typename U>
	void processTwoImg_tmpl2(
		T* dstPtr, void* srcVoidPtr, std::function<double(double, double)> func, qint64 count, iAProgress* p)
	{
		U* srcPtr = static_cast<U*>(srcVoidPtr);
		qint64 step = count / 10;
		qint64 lastEmit = 0;
//#pragma omp parallel for		// no (significant) speedup
		for (qint64 i = 0; i < count; ++i)
		{
			dstPtr[i] = static_cast<T>(func(static_cast<double>(dstPtr[i]), static_cast<double>(srcPtr[i])));
			if (p && i > (lastEmit + step)) // && omp_get_thread_num() == 0)	// in case OpenMP used, see https://stackoverflow.com/questions/28050669/can-i-report-progress-for-openmp-tasks
			{
				p->emitProgress(100 * i / count);
				lastEmit = i;
			}
		}
		if (p)
		{
			p->emitProgress(100);
		}
	}

	template <typename T>
	void processTwoImg_tmpl1(void* dstVoidPtr, vtkSmartPointer<vtkImageData> const src, std::function<double(double, double)> func, qint64 count, iAProgress* p)
	{
		T* dstPtr = static_cast<T*>(dstVoidPtr);
		void* srcVoidPtr = src->GetScalarPointer();
		switch (src->GetScalarType())
		{
			case VTK_UNSIGNED_CHAR:      processTwoImg_tmpl2<T, unsigned char>     (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_CHAR:
			case VTK_SIGNED_CHAR:        processTwoImg_tmpl2<T, char>              (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_SHORT:              processTwoImg_tmpl2<T, short>             (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_UNSIGNED_SHORT:     processTwoImg_tmpl2<T, unsigned short>    (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_INT:                processTwoImg_tmpl2<T, int>               (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_UNSIGNED_INT:       processTwoImg_tmpl2<T, unsigned int>      (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_LONG:               processTwoImg_tmpl2<T, long>              (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_UNSIGNED_LONG:      processTwoImg_tmpl2<T, unsigned long>     (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_LONG_LONG:          processTwoImg_tmpl2<T, long long>         (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_UNSIGNED_LONG_LONG: processTwoImg_tmpl2<T, unsigned long long>(dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_FLOAT:              processTwoImg_tmpl2<T, float>             (dstPtr, srcVoidPtr, func, count, p); break;
			case VTK_DOUBLE:             processTwoImg_tmpl2<T, double>            (dstPtr, srcVoidPtr, func, count, p); break;
			default:
			throw std::runtime_error("Invalid datatype in processTwoImg_tmpl1!");
		}
	}

	void processTwoImg(vtkSmartPointer<vtkImageData> img1, vtkSmartPointer<vtkImageData> const img2,
		std::function<double(double, double)> func, iAProgress* p)
	{
		// check for same dimensions/spacing/origin:
		for (int i = 0; i < 3; ++i)
		{
			assert(img1->GetDimensions()[i] == img2->GetDimensions()[i]);
			assert(img1->GetSpacing()[i]    == img2->GetSpacing()[i]);
			assert(img1->GetOrigin()[i]     == img2->GetOrigin()[i]);
		}
		int const* dim = img1->GetDimensions();
		qint64 count = static_cast<qint64>(dim[0]) * static_cast<qint64>(dim[1]) * dim[2];
		void* dstPtr = img1->GetScalarPointer();
		VTK_TYPED_CALL(processTwoImg_tmpl1, img1->GetScalarType(), dstPtr, img2, func, count, p);
	}
}

void fillImage(vtkSmartPointer<vtkImageData> img, double const value, iAProgress* p)
{
	processImg(img, [value](double) -> double { return value; }, p);
}

iAbase_API void multiplyImage(vtkSmartPointer<vtkImageData> img, double value, iAProgress* p)
{
	processImg(img, [value](double x) -> double { return x * value; }, p);
}

void addImages(vtkSmartPointer<vtkImageData> imgDst, vtkSmartPointer<vtkImageData> const imgToAdd, iAProgress* p)
{
	processTwoImg(imgDst, imgToAdd, [](double x, double y) -> double { return x + y; }, p);
}

void storeImage(vtkSmartPointer<vtkImageData> img, QString const & filename, bool useCompression)
{
	iAConnector con;
	con.setImage(img);
	iAITKIO::ScalarPixelType pixelType = con.itkScalarPixelType();
	iAITKIO::writeFile(filename, con.itkImage(), pixelType, useCompression);
}

void readImage(QString const & filename, bool releaseFlag, vtkSmartPointer<vtkImageData>& ptr)
{
	ptr = vtkSmartPointer<vtkImageData>::New();
	iAConnector con;
	iAITKIO::ScalarPixelType pixelType;
	iAITKIO::ImagePointer img = iAITKIO::readFile(filename, pixelType, releaseFlag);
	con.setImage(img);
	// only works with deep copy, not with returning vtkImage
	// assumption: ITK smart pointer goes out of scope, deletes image, and
	// invalidates "linked" vtk image
	ptr->DeepCopy(con.vtkImage());
}

void writeSingleSliceImage(QString const & filename, vtkImageData* img)
{
	QFileInfo fi(filename);
	vtkSmartPointer<vtkImageWriter> writer;
	if ((QString::compare(fi.suffix(), "TIF", Qt::CaseInsensitive) == 0) || (QString::compare(fi.suffix(), "TIFF", Qt::CaseInsensitive) == 0))
	{
		writer = vtkSmartPointer<vtkTIFFWriter>::New();
	}
	else if (QString::compare(fi.suffix(), "PNG", Qt::CaseInsensitive) == 0)
	{
		writer = vtkSmartPointer<vtkPNGWriter>::New();
	}
	else if ((QString::compare(fi.suffix(), "JPG", Qt::CaseInsensitive) == 0) || (QString::compare(fi.suffix(), "JPEG", Qt::CaseInsensitive) == 0))
	{
		writer = vtkJPEGWriter::New();
	}
	else if (QString::compare(fi.suffix(), "BMP", Qt::CaseInsensitive) == 0)
	{
		writer = vtkBMPWriter::New();
	}
	else
	{
		LOG(lvlError, "Could not write image: Filename has an unknown extension!");
		return;
	}
	writer->SetFileName( getLocalEncodingFileName(filename).c_str() );
	writer->SetInputData(img);
	writer->Write();
}

bool isVtkIntegerImage(vtkImageData* img)
{
	return img->GetScalarType() != VTK_FLOAT && img->GetScalarType() != VTK_DOUBLE;
}

void adjustIndexAndSizeToImage(QVariantMap& params, vtkImageData* img)
{
	auto idx = params["Index"].value<QVector<int>>();
	auto size = params["Size"].value<QVector<int>>();
	int const* dim = img->GetDimensions();
	params["Index"] = variantVector<int>({
		clamp(0, dim[0], idx[0]),
		clamp(0, dim[1], idx[1]),
		clamp(0, dim[2], idx[2])
	});
	params["Size"] = variantVector<int>({
		std::min(size[0], dim[0] - idx[0]),
		std::min(size[1], dim[1] - idx[1]),
		std::min(size[2], dim[2] - idx[2])
	});
}


void mapWorldToVoxelCoords(vtkImageData* img, double const* worldCoord, double* voxelCoord)
{
	double const* imgSpacing = img->GetSpacing();
	double const* imgOrigin  = img->GetOrigin();
	int    const* imgExtent  = img->GetExtent();
	// coords will contain voxel coordinates for the given channel
	for (int i = 0; i < 3; ++i)
	{
		voxelCoord[i] = clamp(
			static_cast<double>(imgExtent[i * 2]),
			imgExtent[i * 2 + 1] + 1 - std::numeric_limits<double>::epsilon(),
			(worldCoord[i] - imgOrigin[i]) / imgSpacing[i] + 0.5);	// + 0.5 to correct for BorderOn
	}
	// TODO: check for negative origin images!
}

iAVec3i mapWorldCoordsToIndex(vtkImageData* img, double const* worldCoord)
{
	double voxelCoord[3];
	mapWorldToVoxelCoords(img, worldCoord, voxelCoord);
	return iAVec3i(static_cast<int>(voxelCoord[0]), static_cast<int>(voxelCoord[1]), static_cast<int>(voxelCoord[2]));
}

size_t mapVTKTypeToSize(int vtkType)
{
	switch (vtkType)
	{
	case VTK_UNSIGNED_CHAR: return sizeof(unsigned char);
	case VTK_SIGNED_CHAR:
#if __cplusplus >= 201703L
		[[fallthrough]];
#endif
	case VTK_CHAR:			return sizeof(char);
	case VTK_UNSIGNED_SHORT:return sizeof(unsigned short);
	case VTK_SHORT:			return sizeof(short);
	case VTK_UNSIGNED_INT:	return sizeof(unsigned int);
	case VTK_INT:			return sizeof(int);
	case VTK_UNSIGNED_LONG:	return sizeof(unsigned long);
	case VTK_LONG:          return sizeof(long);
	case VTK_UNSIGNED_LONG_LONG:return sizeof(unsigned long long);
	case VTK_LONG_LONG:     return sizeof(long long);
	case VTK_FLOAT:         return sizeof(float);
	case VTK_DOUBLE:        return sizeof(double);
	default:                return 0;
	}
}

namespace
{
	QMap<int, QString> const & readableDataTypeMap()
	{
		static QMap<int, QString> nameVTKTypeMap{
			{VTK_UNSIGNED_CHAR     , "8 bit unsigned integer (0 to 255, unsigned char)"},
			{VTK_SIGNED_CHAR       , "8 bit signed integer (-128 to 127, char)"},
			{VTK_UNSIGNED_SHORT    , "16 bit unsigned integer (0 to 65,535, unsigned short)"},
			{VTK_SHORT             , "16 bit signed integer (-32,768 to 32,767, short)"},
			{VTK_UNSIGNED_INT      , "32 bit unsigned integer (0 to 4,294,967,295, unsigned int)"},
			{VTK_INT               , "32 bit signed integer (-2,147,483,648 to 2,147,483,64, int)"},
			{VTK_UNSIGNED_LONG_LONG, "64 bit unsigned integer (0 to (2^64)-1, unsigned long long)"},
			{VTK_LONG_LONG         , "64 bit signed integer (-2^63 to (2^63)-1, long long)"},
			{VTK_FLOAT             , "32 bit floating point number (7 digits, float)"},
			{VTK_DOUBLE            , "64 bit floating point number (15 digits, double)"}
		};
		return nameVTKTypeMap;
	}
}

QStringList const & readableDataTypeList(bool withLongLongTypes)
{   // ToDo: sort data types, otherwise key valus have influence on ordering, which is not ideal (VTK_SIGNED_CHAR at end, not after VTK_UNSIGNED_CHAR)
	static QStringList longlongDataTypeList(readableDataTypeMap().values());
	// exclude all lines matching "long long" from the datatypeList
	static QStringList datatypeList(longlongDataTypeList.filter(QRegularExpression("^((?!long long).)*$")));
	return withLongLongTypes? longlongDataTypeList : datatypeList;
}

int mapReadableDataTypeToVTKType(QString const& dataTypeName)
{
	return readableDataTypeMap().key(dataTypeName, -1);
}

QString mapVTKTypeToReadableDataType(int vtkType)
{
	// map aliases to the values contained in the map:
	if (vtkType == VTK_CHAR)          vtkType = VTK_SIGNED_CHAR;
	if (vtkType == VTK_LONG)          vtkType = VTK_INT;
	if (vtkType == VTK_UNSIGNED_LONG) vtkType = VTK_UNSIGNED_INT;
	// look up type in map:
	return readableDataTypeMap().value(vtkType, "");
}

namespace ByteOrder
{
	const QString BigEndianStr("Big Endian");
	const QString LittleEndianStr("Little Endian");

	QStringList const& stringList()
	{
		static QStringList byteOrders = (QStringList() << BigEndianStr << LittleEndianStr);
		return byteOrders;
	}

	QString mapVTKTypeToString(int byteOrder)
	{
		return (byteOrder == VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN) ? LittleEndianStr : BigEndianStr;
	}

	int mapStringToVTKType(QString const& name)
	{
		return (name == LittleEndianStr) ? VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN : VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
	}
}

QMap<int, QString> const & RenderModeMap()
{
	static QMap<int, QString> renderModeMap;
	if (renderModeMap.isEmpty())
	{
		renderModeMap.insert(vtkSmartVolumeMapper::DefaultRenderMode, "Default (GPU if available, else Software)");
		renderModeMap.insert(vtkSmartVolumeMapper::RayCastRenderMode, "Software Ray-Casting");
		renderModeMap.insert(vtkSmartVolumeMapper::GPURenderMode, "GPU");
#if VTK_OSPRAY_AVAILABLE
		renderModeMap.insert(vtkSmartVolumeMapper::OSPRayRenderMode, "OSPRay");
#endif
	}
	return renderModeMap;
}

int mapRenderModeToEnum(QString const & modeName)
{
	for (int key : RenderModeMap().keys())
		if (RenderModeMap()[key] == modeName)
			return key;

	return vtkSmartVolumeMapper::DefaultRenderMode;
}

void setCamPosition(vtkCamera* cam, iACameraPosition pos)
{
	switch (pos)
	{
	case iACameraPosition::PX:
		cam->SetViewUp(0, 0, 1); cam->SetPosition(1, 0, 0); break;
	case iACameraPosition::MX:
		cam->SetViewUp(0, 0, 1); cam->SetPosition(-1, 0, 0); break;
	case iACameraPosition::PY:
		cam->SetViewUp(0, 0, 1); cam->SetPosition(0, 1, 0); break;
	case iACameraPosition::MY:
		cam->SetViewUp(0, 0, 1); cam->SetPosition(0, -1, 0); break;
	case iACameraPosition::PZ:
		cam->SetViewUp(0, 1, 0); cam->SetPosition(0, 0, 1); break;
	case iACameraPosition::MZ:
		cam->SetViewUp(0, 1, 0); cam->SetPosition(0, 0, -1); break;
	case iACameraPosition::Iso:
		cam->SetViewUp(0, 0, 1); cam->SetPosition(1, 1, 1); break;
	}
	cam->SetFocalPoint(0, 0, 0);
}



void convertLUTToTF(vtkSmartPointer<vtkLookupTable> lut, vtkSmartPointer<vtkColorTransferFunction> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, double alphaOverride)
{
	ctf->RemoveAllPoints();
	otf->RemoveAllPoints();
	double const* range = lut->GetRange();
	for (long long i = 0; i < lut->GetNumberOfColors(); ++i)
	{
		double const* rgba = lut->GetTableValue(i);
		double value = mapValue(0ll, lut->GetNumberOfColors() - 1, range[0], range[1], i);
		ctf->AddRGBPoint(value, rgba[0], rgba[1], rgba[2]);
		otf->AddPoint(value, alphaOverride >= 0 ? alphaOverride : rgba[3]);
	}
}

void convertTFToLUT(vtkSmartPointer<vtkLookupTable> lut, vtkSmartPointer<vtkScalarsToColors> ctf,
	vtkSmartPointer<vtkPiecewiseFunction> otf, int numCols, double const* lutRange, bool reverse)
{
	double rgb[3];
	double const* inRange = ctf->GetRange();
	double const* outRange = lutRange ? lutRange : inRange;
#if VTK_VERSION_NUMBER <= VTK_VERSION_CHECK(8, 0, 0)
	double rangeNonConst[2];
	std::copy(outRange, outRange + 2, rangeNonConst);
	lut->SetRange(rangeNonConst);
	lut->SetTableRange(rangeNonConst);
#else
	lut->SetRange(outRange);
	lut->SetTableRange(outRange);
#endif
	lut->SetNumberOfTableValues(numCols);
	for (long long i = 0; i < numCols; ++i)
	{
		double value = mapValue(0ll, lut->GetNumberOfColors() - 1, inRange[0], inRange[1], i);
		ctf->GetColor(value, rgb);
		double alpha = otf ? otf->GetValue(value) : 1.0;
		lut->SetTableValue( reverse ? numCols - 1 - i : i, rgb[0], rgb[1], rgb[2], alpha);
	}
	lut->Build();
}

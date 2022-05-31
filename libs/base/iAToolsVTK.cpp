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
#include "iALog.h"
#include "iAVtkDraw.h"
#include "iAITKIO.h"

#include <vtkBMPWriter.h>
#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkImageWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkObjectFactory.h>
#include <vtkPNGWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

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

void fillImage(vtkSmartPointer<vtkImageData> img, double const value)
{
	// measure performance + improve!
	// iATypedCallHelper together with std::fill / memset maybe?
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		img->SetScalarComponentFromDouble(x, y, z, 0, value);
	}
}

void addImages(vtkSmartPointer<vtkImageData> imgDst, vtkSmartPointer<vtkImageData> const imgToAdd)
{
	// check for same dimensions/spacing/origin:
	for (int i=0; i<3; ++i)
	{
		assert(imgDst->GetDimensions()[i] == imgToAdd->GetDimensions()[i]);
		assert(imgDst->GetSpacing()[i] == imgToAdd->GetSpacing()[i]);
		assert(imgDst->GetOrigin()[i] == imgToAdd->GetOrigin()[i]);
	}
	FOR_VTKIMG_PIXELS(imgDst, x, y, z)
	{
		imgDst->SetScalarComponentFromDouble(x, y, z, 0,
			imgDst->GetScalarComponentAsDouble(x, y, z, 0) + imgToAdd->GetScalarComponentAsDouble(x, y, z, 0));
	}
}

iAbase_API void multiplyImage(vtkSmartPointer<vtkImageData> imgDst, double value)
{
	FOR_VTKIMG_PIXELS(imgDst, x, y, z)
	{
		imgDst->SetScalarComponentFromDouble(x, y, z, 0, imgDst->GetScalarComponentAsDouble(x, y, z, 0) * value);
	}
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
{
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

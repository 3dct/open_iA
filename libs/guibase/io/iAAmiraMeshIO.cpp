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
#include "iAAmiraMeshIO.h"

// base
#include <iALog.h>
#include <iAFileUtils.h>

#include <vtkImageData.h>

#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <stdexcept>    // for runtime_error


// based on code found here:
// https://people.mpi-inf.mpg.de/~weinkauf/notes/amiramesh.html

const int VTKLabelType = VTK_UNSIGNED_CHAR;
typedef unsigned char LabelType;

//! Find a string in the given buffer and return a pointer to the contents
//! directly behind the SearchString.
//! If not found, return the buffer. A subsequent sscanf()
//! will fail then, but at least we return a decent pointer.
const char* FindAndJump(const char* buffer, const char* SearchString)
{
	const char* FoundLoc = strstr(buffer, SearchString);
	if (FoundLoc) return FoundLoc + strlen(SearchString);
	return buffer;
}

typedef char RawDataType;

int decodeRLE(RawDataType* in, size_t inLength, RawDataType* out, size_t maxOutLength)
{
	size_t curOutStart = 0;
	for (size_t curInIdx = 0; curInIdx < inLength; ++curInIdx)
	{
		int len = in[curInIdx];  // block length
		char c = in[curInIdx + 1]; // character

		if (c < 0)
		{
			c &= 0x7F;
		}

		if (len == EOF) return 1; // end of file
		if (c == EOF) return 0;   // bad format

		if ((curOutStart + len) >= maxOutLength)
		{
			LOG(lvlWarn, "decodeRLE: More data in encoded array than fits into output!");
			break;
		}

		for (int curSubOutIdx = 0; curSubOutIdx < len; curSubOutIdx++)
		{
			out[curOutStart + curSubOutIdx] = c;
		}
		curOutStart += len;
	}
	assert(curOutStart <= std::numeric_limits<int>::max());
	return static_cast<int>(curOutStart);
}

namespace
{
	QString const AmiraMeshFileTag("# AmiraMesh BINARY-LITTLE-ENDIAN 2.1");
	QString const AvizoFileTag("# Avizo BINARY-LITTLE-ENDIAN 2.1");
	const char * DefineLatticeToken = "define Lattice";
	const char * BoundingBoxToken = "BoundingBox";
	QString const ByteType("byte");
	QString const FloatType("float");
}

vtkSmartPointer<vtkImageData> iAAmiraMeshIO::Load(QString const & fileName)
{
	FILE* fp = fopen( getLocalEncodingFileName(fileName).c_str(), "rb");
	if (!fp)
	{
		throw std::runtime_error(QString("Could not open file '%1'.").arg(fileName).toStdString());
	}

	//We read the first 2k bytes into memory to parse the header.
	//The fixed buffer size looks a bit like a hack, and it is one, but it gets the job done.
	const size_t MaxHeaderSize = 2047;
	char buffer[MaxHeaderSize +1];
	size_t readBytes = fread(buffer, sizeof(char), MaxHeaderSize, fp);

	if (readBytes == 0 || ferror(fp) != 0)
	{
		fclose(fp);
		throw std::runtime_error(QString("Could not read header of Avizo/AmiraMesh file %1.").arg(MaxHeaderSize).arg(fileName).toStdString());
	}
	buffer[readBytes-1] = '\0'; //The following string routines prefer null-terminated strings

	QString header(buffer);

	if (!header.startsWith(AmiraMeshFileTag) &&
		!header.startsWith(AvizoFileTag))
	{
		fclose(fp);
		throw std::runtime_error(QString("File %1 is not a proper Avizo/AmiraMesh file, it is missing the initial file tag.").arg(fileName).toStdString());
	}

	//Find the Lattice definition, i.e., the dimensions of the uniform grid
	int xDim(0), yDim(0), zDim(0);
	sscanf(FindAndJump(buffer, DefineLatticeToken), "%d %d %d", &xDim, &yDim, &zDim);
	//LOG(lvlInfo, QString("Grid Dimensions: %1 %2 %3").arg(xDim).arg(yDim).arg(zDim));

	//Find the BoundingBox
	float xmin(1.0f), ymin(1.0f), zmin(1.0f);
	float xmax(-1.0f), ymax(-1.0f), zmax(-1.0f);
	sscanf(FindAndJump(buffer, BoundingBoxToken), "%g %g %g %g %g %g", &xmin, &xmax, &ymin, &ymax, &zmin, &zmax);
	//LOG(lvlInfo, QString("BoundingBox: x=[%1...%2], y=[%3...%4], z=[%5...%6]")
	//	.arg(xmin).arg(xmax).arg(ymin).arg(ymax).arg(zmin).arg(zmax));

	//Is it a uniform grid? We need this only for the sanity check below.
	const bool bIsUniform = (strstr(buffer, "CoordType \"uniform\"") != nullptr);
	//LOG(lvlInfo, QString("GridType: %1").arg(bIsUniform ? "uniform" : "UNKNOWN"));

	//Type of the field: scalar, vector
	int NumComponents(0);

	int latticePos = header.indexOf("Lattice {");
	int nextLineBreakPos = header.indexOf("\n", latticePos);
	int lineSize = nextLineBreakPos - latticePos;
	QString latticeLine = header.mid(latticePos, lineSize );
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	QStringList latticeTokens = latticeLine.split(" ", QString::SkipEmptyParts);
#else
	QStringList latticeTokens = latticeLine.split(" ", Qt::SkipEmptyParts);
#endif

	// TODO more types?
	int dataType;
	QString dataTypeStr = latticeTokens[2];
	if (dataTypeStr == ByteType)
	{
		NumComponents = 1;
		dataType = VTKLabelType;
	}
	else if (dataTypeStr == FloatType)
	{
		NumComponents = 1;
		dataType = VTK_FLOAT;
	}
	else if (dataTypeStr.startsWith(FloatType))
	{
		//A field with more than one component, i.e., a vector field
		dataType = VTK_FLOAT;
		sscanf(FindAndJump(buffer, "Lattice { float["), "%d", &NumComponents);
	}
	else
	{
		fclose(fp);
		throw std::runtime_error(QString("Unknown pixel type '%1' (not yet implemented). Supported pixel types: byte (unsigned char), float.").arg(dataTypeStr).toStdString());
	}
	const QString RLEMarker("HxByteRLE");
	bool rleEncoded = latticeLine.contains(RLEMarker);

	size_t rawDataSize = 0;
	if (rleEncoded)
	{
		if (latticeTokens.size() < 6)
		{
			LOG(lvlWarn, QString("Expected at least 6 tokens in lattice line, only found %1.").arg(latticeTokens.size()));
		}
		int pos = latticeTokens[5].indexOf(RLEMarker);
		//int latticeLength = latticeTokens[5].length();
		int sizePos = pos + RLEMarker.length() + 1;
		int sizeLen = latticeTokens[5].length() - pos - RLEMarker.length() - 2;
		QString dataLenStr = latticeTokens[5].mid(sizePos, sizeLen);
		rawDataSize = dataLenStr.toInt();
		LOG(lvlInfo, QString("RLE encoded (%1 compressed bytes)").arg(rawDataSize));
	}
	//LOG(lvlInfo, QString("Number of Components: %1").arg(NumComponents));

	vtkImageData* imageData = vtkImageData::New();
	imageData->SetDimensions(xDim, yDim, zDim);
	imageData->AllocateScalars(dataType, NumComponents);

	//Sanity check
	if (xDim <= 0 || yDim <= 0 || zDim <= 0
		|| xmin > xmax || ymin > ymax || zmin > zmax
		|| !bIsUniform || NumComponents <= 0)
	{
		fclose(fp);
		throw std::runtime_error("Something went wrong (dimensions smaller or equal 0, [xyz]min > [xyz]max, not uniform or numComponents <= 0).");
	}
	//Find the beginning of the data section
	const long idxStartData = strstr(buffer, "# Data section follows") - buffer;
	if (idxStartData <= 0)
	{
		fclose(fp);
		throw std::runtime_error("Data section not found!");
	}
	//Set the file pointer to the beginning of "# Data section follows"
	bool err = fseek(fp, idxStartData, SEEK_SET) != 0;
	//Consume this line, which is "# Data section follows"
	err |= fgets(buffer, MaxHeaderSize, fp) == nullptr;
	//Consume the next line, which is "@1"
	err |= fgets(buffer, MaxHeaderSize, fp) == nullptr;
	if (err)
	{
		fclose(fp);
		throw std::runtime_error("A read error occured while seeking data section!");
	}

	//Read the data
	// - how much to read
	size_t numOfValues = xDim * yDim * zDim * NumComponents;

	int dataTypeSize = 0;
	switch (dataType)
	{
		case VTK_FLOAT: {
			dataTypeSize = sizeof(float);
			break;
		}
		case VTKLabelType: {
			dataTypeSize = sizeof(LabelType);
			break;
		}
	}
	size_t dataMemorySize = dataTypeSize * numOfValues;
	if (!rawDataSize)
		rawDataSize = dataMemorySize;
	RawDataType* rawData = new RawDataType[rawDataSize];
	size_t rawDataTypeSize = sizeof(RawDataType);

	if (!rawData)
	{
		fclose(fp);
		throw std::runtime_error(QString("Could not allocate memory (%1 bytes)!").arg(rawDataSize).toStdString());
	}
	size_t actRead = fread(
		(void*)rawData, rawDataTypeSize, rawDataSize, fp);

	if (rawDataSize != actRead)
	{
		delete [] rawData;
		fclose(fp);
		throw std::runtime_error(QString("Wanted to read %1 but got %2 bytes while reading the binary data section."
			" Premature end of file?").arg(rawDataSize).arg(actRead).toStdString());
	}

	if (rleEncoded)
	{
		char* output = new RawDataType[dataMemorySize];
		actRead = decodeRLE(rawData, actRead, output, dataMemorySize);
		delete[] rawData;

		if (actRead != dataMemorySize)
		{
			delete[] output;
			fclose(fp);
			throw std::runtime_error(QString("RLE decode: Wanted to get %1 but got %2 bytes while decoding. Wrong data type?").arg(dataMemorySize).arg(actRead).toStdString());
		}

		rawDataSize = dataMemorySize;
		rawData = output;
	}

	//Note: Data runs x-fastest, i.e., the loop over the x-axis is the innermost
	size_t Idx(0);
	for (int z = 0; z<zDim; z++)
	{
		for (int y = 0; y<yDim; y++)
		{
			for (int x = 0; x<xDim; x++)
			{
				//Note: Random access to the value (of the first component) of the grid point (x,y,z):
				// pData[((z * yDim + y) * xDim + x) * NumComponents]
				assert(((static_cast<size_t>(z) * yDim + y) * xDim + x) * NumComponents == Idx * NumComponents);
				for (int c = 0; c<NumComponents; c++)
				{
					float pixelValue = 0;
					switch (dataType)
					{
						case VTK_FLOAT:
							pixelValue = (reinterpret_cast<float*>(rawData))[Idx * NumComponents + c];
							break;
						case VTKLabelType:
							pixelValue = (reinterpret_cast<LabelType*>(rawData))[Idx * NumComponents + c];
							break;
					}
					//printf("%g ", pData[Idx * NumComponents + c]);
					imageData->SetScalarComponentFromFloat(x, y, z, c, pixelValue);
				}
				Idx++;
			}
		}
	}
	delete[] rawData;
	fclose(fp);
	return imageData;
}

void iAAmiraMeshIO::Write(QString const & filename, vtkImageData* img)
{
	int extent[6];
	img->GetExtent(extent);
	int w = extent[1] - extent[0] + 1;
	int h = extent[3] - extent[2] + 1;
	int d = extent[5] - extent[4] + 1;
	int NumComponents = img->GetNumberOfScalarComponents();

	int vtkType = img->GetScalarType();
	QString amiraType;
	QString amiraTypeDesc;
	switch (vtkType)
	{
	case VTK_UNSIGNED_CHAR:
		amiraType = "byte";
		amiraTypeDesc = "Label";
		break;
	case VTK_FLOAT:
		amiraType = "float";
		amiraTypeDesc = "Data";
		break;
	default:
		throw std::runtime_error("Avizo/AmiraMesh: (Currently) unsupported data type! Supported data types: label image (unsigned char), float.");
	}

	QFile file(filename);
	file.open(QIODevice::WriteOnly);

	QByteArray headerData;

	QTextStream stream(&headerData);
	stream << AmiraMeshFileTag << "\n\n\n";
	stream << QString(QString(DefineLatticeToken) + " %1 %2 %3").arg(w).arg(h).arg(d).toLocal8Bit() << "\n\n";
	stream << QString("Parameters {\n"
		//"    Colormap \"labels.am\",\n"
		"    Content \"%1x%2x%3 %4, uniform coordinates\",\n")
		.arg(w).arg(h).arg(d).arg(amiraType).toLocal8Bit();
	stream << QString(
		"    BoundingBox %1 %2 %3 %4 %5 %6,\n"
		"    CoordType \"uniform\"\n"
		"}\n\n")
		.arg(extent[0]).arg(extent[1]).arg(extent[2]).arg(extent[3]).arg(extent[4]).arg(extent[5]).toLocal8Bit();
	stream << QString("Lattice { %1 %2 } @1\n\n").arg(amiraType).arg(amiraTypeDesc);
	stream << "# Data section follows\n";
	stream << "@1\n";
	stream.flush();

	file.write(headerData);
	for (int z = 0; z < d; z++)
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				for (int c = 0; c < NumComponents; c++)
				{
					float pixelValue = img->GetScalarComponentAsFloat(x, y, z, c);
					switch (vtkType)
					{
					case VTK_FLOAT:
					{
						file.write(reinterpret_cast<char*>(&pixelValue), sizeof(float));
						break;
					}
					case VTK_UNSIGNED_CHAR:
					{
						unsigned char pixVal = static_cast<unsigned char>(pixelValue);
						file.write(reinterpret_cast<char*>(&pixVal), 1);
						break;
					}
					}
				}
			}
		}
	}
	file.close();
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAmiraMeshIO.h"

// base
#include <iALog.h>

#include <vtkImageData.h>

#include <QFile>
#include <QRegularExpression>
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
QString extractValues(QString str, QString identifier, QString finalizer)
{
	auto startPos = str.indexOf(identifier) + identifier.length();
	auto endPos = str.indexOf(QRegularExpression(finalizer), startPos);
	return str.mid(startPos, endPos-startPos);
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
	QString const DefineLatticeToken("define Lattice ");
	QString const BoundingBoxToken("BoundingBox ");
	QString const ByteType("byte");
	QString const FloatType("float");
}

vtkSmartPointer<vtkImageData> iAAmiraMeshIO::Load(QString const & fileName)
{
	const size_t MaxHeaderSize = 2047;
	QFile f(fileName);
	if (!f.open(QFile::ReadOnly | QFile::Text))
	{
		throw std::runtime_error(QString("Could not open file '%1'.").arg(fileName).toStdString());
	}
	QTextStream in(&f);
	auto header = in.read(MaxHeaderSize);
	auto readBytes = header.length();
	if (readBytes == 0)
	{
		throw std::runtime_error(QString("Could not read header of Avizo/AmiraMesh file %1.").arg(MaxHeaderSize).arg(fileName).toStdString());
	}
	if (!header.startsWith(AmiraMeshFileTag) &&
		!header.startsWith(AvizoFileTag))
	{
		throw std::runtime_error(QString("File %1 is not a proper Avizo/AmiraMesh file, it is missing the initial file tag.").arg(fileName).toStdString());
	}

	//Find the Lattice definition, i.e., the dimensions of the uniform grid
	auto dims = extractValues(header, DefineLatticeToken, "[\r\n]");
	auto dimStrs = dims.split(" ");
	auto xDim = dimStrs[0].toInt();
	auto yDim = dimStrs[1].toInt();
	auto zDim = dimStrs[2].toInt();

	//Find the BoundingBox
	auto bbs = extractValues(header, BoundingBoxToken, "[\r\n]");
	auto bbStrs = bbs.split(" ");
	auto xmin = bbStrs[0].toFloat();
	auto xmax = bbStrs[1].toFloat();
	auto ymin = bbStrs[2].toFloat();
	auto ymax = bbStrs[3].toFloat();
	auto zmin = bbStrs[4].toFloat();
	auto zmax = bbStrs[5].toFloat();

	//Is it a uniform grid? We need this only for the sanity check below.
	const bool bIsUniform = header.contains("CoordType \"uniform\"");

	//Type of the field: scalar, vector
	int NumComponents(0);

	auto latticePos = header.indexOf("Lattice {");
	auto nextLineBreakPos = header.indexOf("\n", latticePos);
	auto lineSize = nextLineBreakPos - latticePos;
	QString latticeLine = header.mid(latticePos, lineSize );
	QStringList latticeTokens = latticeLine.split(" ", Qt::SkipEmptyParts);

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
		auto numComponentStr = extractValues(header, "Lattice { float[", "\\]");
		NumComponents = numComponentStr.toInt();
	}
	else
	{
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
		auto pos = latticeTokens[5].indexOf(RLEMarker);
		auto sizePos = pos + RLEMarker.length() + 1;
		auto sizeLen = latticeTokens[5].length() - pos - RLEMarker.length() - 2;
		QString dataLenStr = latticeTokens[5].mid(sizePos, sizeLen);
		rawDataSize = dataLenStr.toInt();
		LOG(lvlInfo, QString("RLE encoded (%1 compressed bytes)").arg(rawDataSize));
	}

	vtkImageData* imageData = vtkImageData::New();
	imageData->SetDimensions(xDim, yDim, zDim);
	imageData->AllocateScalars(dataType, NumComponents);

	//Sanity check
	if (xDim <= 0 || yDim <= 0 || zDim <= 0
		|| xmin > xmax || ymin > ymax || zmin > zmax
		|| !bIsUniform || NumComponents <= 0)
	{
		throw std::runtime_error("Something went wrong (dimensions smaller or equal 0, [xyz]min > [xyz]max, not uniform or numComponents <= 0).");
	}
	//Find the beginning of the data section
	const long idxStartData = header.indexOf("# Data section follows");
	if (idxStartData <= 0)
	{
		throw std::runtime_error("Data section not found!");
	}
	FILE* fp = fopen(fileName.toStdString().c_str(), "rb");
	char buffer[MaxHeaderSize + 1];
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
	{
		rawDataSize = dataMemorySize;
	}
	RawDataType* rawData = new RawDataType[rawDataSize];
	size_t rawDataTypeSize = sizeof(RawDataType);

	if (!rawData)
	{
		fclose(fp);
		throw std::runtime_error(QString("Could not allocate memory (%1 bytes)!").arg(rawDataSize).toStdString());
	}
	size_t actRead = fread(	(void*)rawData, rawDataTypeSize, rawDataSize, fp);

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

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
#include "iAVGIFileIO.h"

#include "iAFileUtils.h"
#include "iAProgress.h"
#include "iAValueTypeVectorHelpers.h"
#include "iAToolsVTK.h"

#include "iARawFileIO.h"

#include <vtkImageData.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

iAVGIFileIO::iAVGIFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::None)
{}

namespace
{
	// TODO: rewrite to read file only once, return vector for multi-entry types (size, spacing, ...)
	QString getParameterValues(QString fileName, QString parameter, int index, QString section = "", QString sep = ":", bool caseSensitive = true)
	{
		if (index < 0 || index>3)
		{
			return 0;
		}
		QString values[4];
		QFile file(fileName);

		if (file.open(QIODevice::ReadOnly))
		{
			QTextStream textStream(&file);
			QString currentLine, currentSection;
			while (!textStream.atEnd())
			{
				currentLine = textStream.readLine();
				if (!currentLine.isEmpty())
				{
					QString currentParameter;
					if ((currentLine.indexOf("[") == 0) && (currentLine.indexOf("[") < currentLine.indexOf("]")))
						currentSection = currentLine.section(" ", 0, 0);
					else
						currentParameter = currentLine.section(" ", 0, 0);

					if ((section != "") && (!currentSection.startsWith(section))) continue;
					else
					{
						if (currentParameter.startsWith(parameter, caseSensitive ? Qt::CaseSensitive: Qt::CaseInsensitive))
						{
							QString temp = currentLine.remove(0, currentLine.indexOf(sep) + 1);
							temp = temp.simplified();
							if (currentParameter == "name")
							{
								values[index] = temp;
								return values[index];
							}

							values[0] = temp.section(" ", 0, 0).trimmed();
							values[1] = temp.section(" ", 1, 1).trimmed();
							values[2] = temp.section(" ", 2, 2).trimmed();

							if (currentLine.indexOf("%") >= 0)
								values[3] = temp.section("%", 1, 1);
						}
					}
				}
			}
			file.close();
		}
		else return "";

		return values[index];
	}
}

std::vector<std::shared_ptr<iADataSet>> iAVGIFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress* progress)
{
	Q_UNUSED(paramValues);

	// TODO: rewrite to read file only once!
	QVariantMap rawFileParams;
	int sizeX = getParameterValues(fileName, "size", 0, "[file1]", "=", false).toInt();
	int sizeY = getParameterValues(fileName, "size", 1, "[file1]", "=", false).toInt();
	int sizeZ = getParameterValues(fileName, "size", 2, "[file1]", "=", false).toInt();
	if (sizeX == 0 || sizeY == 0 || sizeZ == 0)
	{
		LOG(lvlError, QString("VGI reader: One of the 3 dimensions has size 0 (determined values: %1x%2x%3)!").arg(sizeX).arg(sizeY).arg(sizeZ));
		return {};
	}
	rawFileParams[iARawFileIO::SizeStr] = variantVector<int>({ sizeX, sizeY, sizeZ });
	double spacingX = getParameterValues(fileName, "resolution", 0, "[geometry]", "=", false).toDouble();
	double spacingY = getParameterValues(fileName, "resolution", 1, "[geometry]", "=", false).toDouble();
	double spacingZ = getParameterValues(fileName, "resolution", 2, "[geometry]", "=", false).toDouble();
	if (spacingY == 0 && spacingZ == 0)
	{
		spacingY = spacingX;
		spacingZ = spacingX;
	}
	if (spacingX == 0 || spacingY == 0 || spacingZ == 0)
	{
		spacingX = 1;
		spacingY = 1;
		spacingZ = 1;
	}
	rawFileParams[iARawFileIO::SpacingStr] = variantVector<double>({ spacingX, spacingY, spacingZ });
	double originX = getParameterValues(fileName, "position", 0, "[geometry]", "=", false).toDouble();
	double originY = getParameterValues(fileName, "position", 1, "[geometry]", "=", false).toDouble();
	double originZ = getParameterValues(fileName, "position", 2, "[geometry]", "=", false).toDouble();
	if (originX == 0 || originY == 0 || originZ == 0)
	{
		originX = 1;
		originY = 1;
		originZ = 1;
	}
	rawFileParams[iARawFileIO::OriginStr] = variantVector<double>({ originX, originY, originZ });

	int	elementSize = getParameterValues(fileName, "BitsPerElement", 0, "[file1]", "=", false).toInt();
	if (elementSize == 0)
	{
		LOG(lvlError, "VGI reader: BitsPerElement is 0 / not set!");
		return {};
	}
	if (elementSize == 8)
	{
		rawFileParams[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(VTK_UNSIGNED_CHAR);
	}
	else if (elementSize == 16)
	{
		rawFileParams[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(VTK_UNSIGNED_SHORT);
	}
	else if (elementSize == 32)
	{
		rawFileParams[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(VTK_FLOAT);
	}
	else
	{
		LOG(lvlError, QString("VGI reader: Not known to what data type to map an element size of %1!").arg(elementSize));
		return {};
	}

	rawFileParams[iARawFileIO::HeadersizeStr] = getParameterValues(fileName, "SkipHeader", 0, "[file1]", "=", false).toInt();
	rawFileParams[iARawFileIO::ByteOrderStr] = ByteOrder::LittleEndianStr;

	auto rawFileName = getParameterValues(fileName, "Name", 0, "[file1]", "=", false);
	if (rawFileName.isEmpty())
	{
		LOG(lvlError, "VGI reader: Data file path is empty!");
		return {};
	}
	QFileInfo fi(fileName);
	if (!QFile::exists(rawFileName))
	{
		if ((rawFileName.lastIndexOf("\\") == -1) && (rawFileName.lastIndexOf("/") == -1))
		{
			rawFileName = fi.canonicalPath() + "/" + rawFileName;
		}
		else if (rawFileName.lastIndexOf("\\") > 0)
		{
			rawFileName = fi.canonicalPath() + "/" + rawFileName.section('\\', -1);
		}
		else if (rawFileName.lastIndexOf("/") > 0)
		{
			rawFileName = fi.canonicalPath() + "/" + rawFileName.section('/', -1);
		}
	}
	if (!QFile::exists(rawFileName))
	{
		LOG(lvlError, QString("VGI reader: Data file path in VGI points to a non-existing file %1").arg(rawFileName));
		return {};
	}
	iARawFileIO io;
	return io.load(rawFileName, rawFileParams, progress);
}

QString iAVGIFileIO::name() const
{
	return "VG Studio Scenes";
}

QStringList iAVGIFileIO::extensions() const
{
	return QStringList{ "vgi" };
}

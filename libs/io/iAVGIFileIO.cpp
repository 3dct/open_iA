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
#include "iAStringHelper.h"
#include "iAToolsVTK.h"
#include "iAValueTypeVectorHelpers.h"

#include "iARawFileIO.h"

#include <vtkImageData.h>

#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

iAVGIFileIO::iAVGIFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::None)
{}

std::shared_ptr<iADataSet> iAVGIFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	//static auto VGIFileFormat = QSettings::registerFormat("vgi", )
	// TODO: rewrite to read file only once!
	QVariantMap rawFileParams;
	QSettings vgiFileSettings(fileName, QSettings::IniFormat);
	auto size = stringToVector<QVector<int>, int>(vgiFileSettings.value("file1/size").toString(), " ");
	size.resize(3);
	if (size[0] == 0 || size[1] == 0 || size[2] == 0)
	{
		LOG(lvlError, QString("VGI reader: One of the 3 dimensions has size 0 (determined values: %1x%2x%3)!").arg(size[0]).arg(size[1]).arg(size[2]));
		return {};
	}
	rawFileParams[iARawFileIO::SizeStr] = QVariant::fromValue(size);
	auto spacing = stringToVector<QVector<double>, double>(vgiFileSettings.value("geometry/resolution").toString(), " ");
	spacing.resize(3);
	if (spacing[1] == 0 && spacing[2] == 0)
	{
		spacing[1] = spacing[0];
		spacing[2] = spacing[0];
	}
	if (spacing[0] == 0 || spacing[1] == 0 || spacing[2] == 0)
	{
		spacing[0] = 1;
		spacing[1] = 1;
		spacing[2] = 1;
	}
	rawFileParams[iARawFileIO::SpacingStr] = QVariant::fromValue(spacing);
	auto origin = stringToVector<QVector<double>, double>(vgiFileSettings.value("geometry/position").toString(), " ");
	origin.resize(3);
	if (origin[0] == 0 || origin[1] == 0 || origin[2] == 0)
	{
		origin[0] = 1;
		origin[1] = 1;
		origin[2] = 1;
	}
	rawFileParams[iARawFileIO::OriginStr] = QVariant::fromValue(origin);
	int	elementSize = vgiFileSettings.value("file1/BitsPerElement", 0).toInt();
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

	rawFileParams[iARawFileIO::HeadersizeStr] = vgiFileSettings.value("file1/SkipHeader").toInt();
	rawFileParams[iARawFileIO::ByteOrderStr] = ByteOrder::LittleEndianStr;

	auto rawFileName = vgiFileSettings.value("file1/Name").toString();
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

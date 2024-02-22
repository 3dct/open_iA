// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	rawFileParams[iARawFileIO::SizeStr] = variantVector(size);
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
	rawFileParams[iARawFileIO::SpacingStr] = variantVector(spacing);
	auto origin = stringToVector<QVector<double>, double>(vgiFileSettings.value("geometry/position").toString(), " ");
	origin.resize(3);
	if (origin[0] == 0 || origin[1] == 0 || origin[2] == 0)
	{
		origin[0] = 1;
		origin[1] = 1;
		origin[2] = 1;
	}
	rawFileParams[iARawFileIO::OriginStr] = variantVector(origin);
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
	rawFileParams[iARawFileIO::ByteOrderStr] = iAByteOrder::LittleEndianStr;

	auto rawFileName = tryFixFileName(vgiFileSettings.value("file1/Name").toString(), QFileInfo(fileName).canonicalPath());
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

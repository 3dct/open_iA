// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAmiraVolumeFileIO.h"

#include "iAAmiraMeshIO.h"
#include "iAImageData.h"

#include <vtkImageData.h>

iAAmiraVolumeFileIO::iAAmiraVolumeFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{}

std::shared_ptr<iADataSet> iAAmiraVolumeFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(progress);
	Q_UNUSED(paramValues);
	return { std::make_shared<iAImageData>(iAAmiraMeshIO::Load(fileName)) };
}

void iAAmiraVolumeFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(progress);
	Q_UNUSED(paramValues);
	if (!dynamic_cast<iAImageData*>(dataSet.get()))
	{
		throw std::runtime_error("Amira Volume export: Given dataset is not an image!");
	}
	iAAmiraMeshIO::Write(fileName, dynamic_cast<iAImageData*>(dataSet.get())->vtkImage().Get());
}

QString iAAmiraVolumeFileIO::name() const
{
	return "Amira volume data";
}

QStringList iAAmiraVolumeFileIO::extensions() const
{
	return QStringList{ "am" };
}

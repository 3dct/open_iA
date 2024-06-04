// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACSVImageFileIO.h"

#include "iAImageData.h"
#include "iAProgress.h"
#include "iAToolsVTK.h"

#include <vtkImageData.h>

#include <fstream>

iACSVImageFileIO::iACSVImageFileIO() : iAFileIO(iADataSetType::None, iADataSetType::Volume)
{
	addAttr(m_params[Save], "Coordinates", iAValueType::Boolean, false);
}

QString iACSVImageFileIO::name() const
{
	return "CSV file";
}

QStringList iACSVImageFileIO::extensions() const
{
	return QStringList{ "csv" };
}

void iACSVImageFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	if (!imgData)
	{
		throw std::runtime_error("CSV Volume export: Given dataset is not an image!");
	}
	auto img = imgData->vtkImage();
	int numberOfComponents = img->GetNumberOfScalarComponents();
	std::ofstream out(fileName.toStdString());
	size_t voxelCount = imgData->voxelCount();
	size_t curVoxel = 0;
	bool coords = paramValues["Coordinates"].toBool();
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		if (coords)
		{
			out << x << "," << y << "," << z << ",";
		}
		for (int c = 0; c < numberOfComponents; ++c)
		{
			out << img->GetScalarComponentAsDouble(x, y, z, 0);
			if (c < numberOfComponents - 1)
			{
				out << ",";
			}
		}
		progress.emitProgress( (100.0 * curVoxel) / voxelCount);
		++curVoxel;
		out << std::endl;
	}
	out.close();
}

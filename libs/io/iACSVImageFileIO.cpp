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
#include "iACSVImageFileIO.h"

#include "iAFileUtils.h"
#include "iAProgress.h"
#include "iAToolsVTK.h"

#include <vtkImageData.h>

#include <fstream>

iACSVImageFileIO::iACSVImageFileIO() : iAFileIO(iADataSetType::None, iADataSetType::Volume)
{
	addAttr(m_params[Load], "Coordinates", iAValueType::Boolean, false);
}

QString iACSVImageFileIO::name() const
{
	return "CSV file";
}

QStringList iACSVImageFileIO::extensions() const
{
	return QStringList{ "csv" };
}

void iACSVImageFileIO::save(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& paramValues, iAProgress* progress)
{
	assert(dataSets.size() == 1);
	auto imgData = dynamic_cast<iAImageData*>(dataSets[0].get());
	auto img = imgData->image();
	int numberOfComponents = img->GetNumberOfScalarComponents();
	std::ofstream out(getLocalEncodingFileName(fileName));
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
		progress->emitProgress( (100.0 * curVoxel) / voxelCount);
		++curVoxel;
		out << std::endl;
	}
	out.close();
}

bool iACSVImageFileIO::s_bRegistered = iAFileTypeRegistry::addFileType<iACSVImageFileIO>();

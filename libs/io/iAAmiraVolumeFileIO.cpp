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
#include "iAAmiraVolumeFileIO.h"

#include "iAAmiraMeshIO.h"

#include <vtkImageData.h>

iAAmiraVolumeFileIO::iAAmiraVolumeFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{}

std::vector<std::shared_ptr<iADataSet>> iAAmiraVolumeFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(progress);
	Q_UNUSED(paramValues);
	return { std::make_shared<iAImageData>(iAAmiraMeshIO::Load(fileName)) };
}

void iAAmiraVolumeFileIO::saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>>& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(progress);
	Q_UNUSED(paramValues);
	if (dataSets.size() != 1)
	{
		throw std::runtime_error("Amira Volume export: Only exactly one dataset supported!");
	}
	if (!dynamic_cast<iAImageData*>(dataSets[0].get()))
	{
		throw std::runtime_error("Amira Volume export: Given dataset is not an image!");
	}
	iAAmiraMeshIO::Write(fileName, dynamic_cast<iAImageData*>(dataSets[0].get())->vtkImage().Get());
}

QString iAAmiraVolumeFileIO::name() const
{
	return "Amira volume data";
}

QStringList iAAmiraVolumeFileIO::extensions() const
{
	return QStringList{ "am" };
}

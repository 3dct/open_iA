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
#include "iAFileIO.h"

const QString iAFileIO::CompressionStr("Compression");

iAFileIO::iAFileIO(iADataSetTypes loadTypes, iADataSetTypes saveTypes) :
	m_loadDataSetTypes(loadTypes), m_saveDataSetTypes(saveTypes)
{}

iAFileIO::~iAFileIO()
{}

std::vector<std::shared_ptr<iADataSet>> iAFileIO::load(QString const& fileName, iAProgress* progress, QVariantMap const& paramValues)
{
	Q_UNUSED(fileName);
	Q_UNUSED(progress);
	Q_UNUSED(paramValues);
	return {};
}

iAAttributes const& iAFileIO::loadParameter() const
{
	return m_loadParams;
}

iADataSetTypes iAFileIO::supportedLoadDataSetTypes() const
{
	return m_loadDataSetTypes;
}

bool iAFileIO::isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const
{
	Q_UNUSED(dataSet);
	Q_UNUSED(fileName);
	return true;
}

void iAFileIO::save(QString const& fileName, iAProgress* progress, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& paramValues)
{
	Q_UNUSED(fileName);
	Q_UNUSED(progress);
	Q_UNUSED(dataSets);
	Q_UNUSED(paramValues);
}

iAAttributes const& iAFileIO::saveParameter() const
{
	return m_saveParams;
}

iADataSetTypes iAFileIO::supportedSaveDataSetTypes() const
{
	return m_saveDataSetTypes;
}

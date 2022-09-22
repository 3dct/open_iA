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

#include "iALog.h"

#include <QElapsedTimer>
#include <QFileInfo>

const QString iAFileIO::CompressionStr("Compression");

iAFileIO::iAFileIO(iADataSetTypes loadTypes, iADataSetTypes saveTypes) :
	m_dataSetTypes{ loadTypes, saveTypes }
{
	addAttr(m_params[Load], iADataSet::FileNameKey, iAValueType::FileNameOpen, "");
}

iAFileIO::~iAFileIO() = default;

std::vector<std::shared_ptr<iADataSet>> iAFileIO::load(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	try
	{
		QElapsedTimer t;
		t.start();
		QVariantMap checkedValues(paramValues);
		checkParams(checkedValues, Operation::Load, fileName);
		auto dataSets = loadData(fileName, checkedValues, progress);

		for (auto d : dataSets)
		{
			d->setMetaData(iADataSet::FileNameKey, fileName);
			d->setMetaData(iADataSet::NameKey, QFileInfo(fileName).completeBaseName());
			for (auto k : checkedValues.keys())
			{
				d->setMetaData(k, checkedValues[k]);
			}
		}
		// for file formats that support multiple dataset types: check if an allowed type was loaded?
		// BUT currently no such format supported
		LOG(lvlInfo, QString("Loaded dataset %1 in %2 ms.").arg(fileName).arg(t.elapsed()));
		return dataSets;
	}
	// TODO: unify exception handling?
	catch (itk::ExceptionObject& e)
	{
		LOG(lvlError, QString("Error loading file %1: %2").arg(fileName).arg(e.GetDescription()));
	}
	catch (std::exception& e)
	{
		LOG(lvlError, QString("Error loading file %1: %2").arg(fileName).arg(e.what()));
	}
	catch (...)
	{
		LOG(lvlError, QString("Unknown error while loading file %1!").arg(fileName));
	}
	return std::vector<std::shared_ptr<iADataSet>>();
}

std::vector<std::shared_ptr<iADataSet>> iAFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(fileName);
	Q_UNUSED(paramValues);
	Q_UNUSED(progress);
	return {};
}

void iAFileIO::saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>>& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(fileName);
	Q_UNUSED(dataSets);
	Q_UNUSED(paramValues);
	Q_UNUSED(progress);
}

iAAttributes const& iAFileIO::parameter(Operation op) const
{
	return m_params[op];
}

iADataSetTypes iAFileIO::supportedDataSetTypes(Operation op) const
{
	return m_dataSetTypes[op];
}

bool iAFileIO::isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const
{
	Q_UNUSED(dataSet);
	Q_UNUSED(fileName);
	return true;
}

void iAFileIO::save(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> & dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	QVariantMap checkedValues(paramValues);
	checkParams(checkedValues, Save, fileName);
	saveData(fileName, dataSets, checkedValues, progress);
	for (auto d : dataSets)
	{
		d->setMetaData(iADataSet::FileNameKey, fileName);
	}
}

bool iAFileIO::checkParams(QVariantMap & paramValues, Operation op, QString const& fileName)
{
	bool result = true;
	paramValues[iADataSet::FileNameKey] = fileName;
	for (auto p : m_params[op])
	{
		// ToDo: maybe also check whether the values fulfill the conditions in the iAAttributeDescriptor? see iAFilter::checkParameters
		if (!paramValues.contains(p->name()))
		{
			LOG(lvlWarn, QString("Missing parameter %1 when %2 file %3; setting to default %4!")
				.arg(p->name()).arg(op == Save ? "saving" : "loading").arg(fileName)
				.arg(p->defaultValue().toString())
			);
			result = false;
		}
	}
	return result;
}

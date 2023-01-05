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
#include "iAProjectFileIO.h"

#include "iAFileTypeRegistry.h"
#include "iAFileUtils.h"
#include "iAProgress.h"
#include "iASettings.h"    // for mapFromQSettings

#include <QFileInfo>
#include <QSettings>

namespace
{
	static const QString ProjectFileVersionKey("FileVersion");
	static const QString ProjectFileVersionValue("1.0");
	// for backwards compatibility, we'll also check "Modality" when loading
	static const QString ProjectFileDataSetOLD("Modality");
	static const QString ProjectFileDataSet("DataSet");

	QString dataSetGroup(int idx, bool old)
	{
		return (old ? ProjectFileDataSetOLD : ProjectFileDataSet) + QString::number(idx);
	}
}

const QString iAProjectFileIO::Name("Project files");

iAProjectFileIO::iAProjectFileIO() : iAFileIO(iADataSetType::Collection, iADataSetType::Collection)
{}

std::shared_ptr<iADataSet> iAProjectFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	QFileInfo fi(fileName);
	if (!fi.exists())
	{
		LOG(lvlError, QString("Given project file '%1' does not exist.").arg(fileName));
		return {};
	}
	auto s = std::make_shared<QSettings>(fileName, QSettings::IniFormat);
	auto &settings = *s.get();

	if (!settings.contains(ProjectFileVersionKey) ||
		settings.value(ProjectFileVersionKey).toString() != ProjectFileVersionValue)
	{
		LOG(lvlError, QString("Invalid project file version (was %1, expected %2)! Trying to parse anyway, but expect failures.")
			.arg(settings.contains(ProjectFileVersionKey) ? settings.value(ProjectFileVersionKey).toString() : "not set")
			.arg(ProjectFileVersionValue));
		return {};
	}

	int maxIdx = 0;
	while (settings.contains(dataSetGroup(maxIdx, true) + "/File") || settings.contains(dataSetGroup(maxIdx, false) + "/File"))
	{
		++maxIdx;
	}

	int currIdx = 0;
	auto result = std::make_shared<iADataCollection>(maxIdx - currIdx, s);
	result->setMetaData(mapFromQSettings(settings));
	while (currIdx < maxIdx)
	{
		settings.beginGroup(dataSetGroup(currIdx, settings.contains(dataSetGroup(maxIdx, true)) ? true: false));
		QString dataSetFileName = MakeAbsolute(fi.absolutePath(), settings.value("File").toString());
		try
		{
			auto io = iAFileTypeRegistry::createIO(dataSetFileName, iAFileIO::Load);
			if (io)
			{
				auto dataSetParamValues = mapFromQSettings(settings);
				auto currentDataSet = io->load(dataSetFileName, dataSetParamValues);
				result->addDataSet(currentDataSet);
			}
		}
		// catch exceptions here to skip only the current dataset on error, don't abort loading the whole project
		catch (itk::ExceptionObject& e)
		{
			LOG(lvlError, QString("Error (ITK) loading file %1: %2").arg(fileName).arg(e.GetDescription()));
		}
		catch (std::exception& e)
		{
			LOG(lvlError, QString("Error loading file %1: %2").arg(fileName).arg(e.what()));
		}
		catch (...)
		{
			LOG(lvlError, QString("Unknown error while loading file %1!").arg(fileName));
		}
		settings.endGroup();
		++currIdx;
		progress.emitProgress(100.0 * currIdx / maxIdx);
	}
	return result;
}


void iAProjectFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	auto collection = dynamic_cast<iADataCollection*>(dataSet.get());
	assert(collection->settings()->fileName() == fileName);
	for (size_t d = 0; d < collection->dataSets().size(); ++d)
	{
		collection->settings()->beginGroup(dataSetGroup(d, false));
		auto ds = collection->dataSets()[d];
		for (auto key : ds->allMetaData().keys())
		{
			collection->settings()->setValue(key, ds->metaData(key));
		}
		collection->settings()->endGroup();
		progress.emitProgress(100.0 * d / collection->dataSets().size());
	}
}

QString iAProjectFileIO::name() const
{
	return Name;
}

QStringList iAProjectFileIO::extensions() const
{
	return QStringList{ "iaproj", "mod" };
}

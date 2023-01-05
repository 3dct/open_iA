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
	static const int ProjectFileVersion = 2;
	// for backwards compatibility, we'll also check "Modality" when loading
	static const QString ProjectFileDataSetOLD("Modality");
	static const QString ProjectFileDataSet("DataSet");

	QString dataSetGroup(size_t idx, bool old)
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
		throw std::runtime_error(QString("Given project file '%1' does not exist.").arg(fileName).toStdString());
	}
	auto s = std::make_shared<QSettings>(fileName, QSettings::IniFormat);
	auto &settings = *s.get();

	if (!settings.contains(ProjectFileVersionKey))
	{
		throw std::runtime_error(QString("Project file %1 does not contain the required %2 key!")
			.arg(fileName).arg(ProjectFileVersionKey).toStdString());
	}
	bool ok;
	double fileVersion = settings.value(ProjectFileVersionKey).toDouble(&ok);
	if (!ok || fileVersion > ProjectFileVersion)
	{
		throw std::runtime_error(QString("Invalid value for project file version (was %1, expected %2 or smaller)!")
			.arg(settings.value(ProjectFileVersionKey).toString())
			.arg(ProjectFileVersion).toStdString());
	}
	size_t numDataSets = 0;
	while (settings.contains(dataSetGroup(numDataSets, true)  + "/" + iADataSet::FileNameKey) ||
		   settings.contains(dataSetGroup(numDataSets, false) + "/" + iADataSet::FileNameKey))
	{
		++numDataSets;
	}
	progress.emitProgress(0);
	int idx = 0;
	auto result = std::make_shared<iADataCollection>(numDataSets, s);
	result->setMetaData(mapFromQSettings(settings));
	while (idx < numDataSets)
	{
		settings.beginGroup(dataSetGroup(idx, settings.contains(dataSetGroup(idx, true)) ? true: false));
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
		++idx;
		progress.emitProgress(100.0 * idx / numDataSets);
	}
	return result;
}


void iAProjectFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	auto collection = dynamic_cast<iADataCollection*>(dataSet.get());
	collection->settings()->setValue(ProjectFileVersionKey, ProjectFileVersion);
	assert(collection->settings()->fileName() == fileName);
	for (size_t d = 0; d < collection->dataSets().size(); ++d)
	{
		collection->settings()->beginGroup(dataSetGroup(d, false));
		auto ds = collection->dataSets()[d];
		if (ds->type() == iADataSetType::Collection)
		{
			LOG(lvlWarn, QString("Will not store collection dataset(% 1)!").arg(ds->name()) );
			continue;
		}
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

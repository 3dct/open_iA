// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAProjectFileIO.h"

#include <iALog.h>
#include <iAFileTypeRegistry.h>
#include <iAFileUtils.h>
#include <iAProgress.h>
#include <iASettings.h>    // for mapFromQSettings

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
	size_t idx = 0;
	auto result = std::make_shared<iADataCollection>(numDataSets, s);
	result->setMetaData(mapFromQSettings(settings));
	while (idx < numDataSets)
	{
		settings.beginGroup(dataSetGroup(idx, settings.contains(dataSetGroup(idx, true) + "/" + iADataSet::FileNameKey) ? true: false));
		QString dataSetFileName = MakeAbsolute(fi.absolutePath(), settings.value("File").toString());
		auto io = iAFileTypeRegistry::createIO(dataSetFileName, iAFileIO::Load);
		if (io)
		{
			auto dataSetParamValues = mapFromQSettings(settings);
			auto currentDataSet = io->load(dataSetFileName, dataSetParamValues);
			if (currentDataSet)
			{
				currentDataSet->setMetaData(dataSetParamValues);
				result->addDataSet(currentDataSet);
			}
			else
			{
				LOG(lvlError, QString("Failed to load dataset %1, skipping...").arg(dataSetFileName));
			}
		}
		settings.endGroup();
		++idx;
		progress.emitProgress(100.0 * idx / numDataSets);
	}
	return result;
}


void iAProjectFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet,
							   QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(fileName)
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
			LOG(lvlWarn, QString("Will not store collection dataset (%1) - unsupported at the moment!").arg(ds->name()) );
			continue;
		}
		for (auto key : ds->allMetaData().keys())
		{
			auto value = ds->metaData(key);
			if (key == iADataSet::FileNameKey)
			{
				value = MakeRelative(QFileInfo(fileName).absolutePath(), value.toString());
			}
			collection->settings()->setValue(key, value);
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

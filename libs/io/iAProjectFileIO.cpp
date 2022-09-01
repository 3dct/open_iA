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

	QString dataSetGroup(int idx)
	{   // for backwardss compatibility, let's keep "Modality" as identifier for now
		return QString("Modality") + QString::number(idx);
	}
}

const QString iAProjectFileIO::Name("Project files");

iAProjectFileIO::iAProjectFileIO() : iAFileIO(iADataSetType::All, iADataSetType::None) // writing to a project file is specific (since it doesn't write the dataset itself...)
{}

std::shared_ptr<iAFileIO> iAProjectFileIO::create()
{
	return std::make_shared<iAProjectFileIO>();
}

std::vector<std::shared_ptr<iADataSet>> iAProjectFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress* progress)
{
	Q_UNUSED(paramValues);
	QFileInfo fi(fileName);
	if (!fi.exists())
	{
		LOG(lvlError, QString("Given project file '%1' does not exist.").arg(fileName));
		return {};
	}
	QSettings settings(fileName, QSettings::IniFormat);
	//iAVolumeSettings volSettings;

	if (!settings.contains(ProjectFileVersionKey) ||
		settings.value(ProjectFileVersionKey).toString() != ProjectFileVersionValue)
	{
		LOG(lvlError, QString("Invalid project file version (was %1, expected %2)! Trying to parse anyway, but expect failures.")
			.arg(settings.contains(ProjectFileVersionKey) ? settings.value(ProjectFileVersionKey).toString() : "not set")
			.arg(ProjectFileVersionValue));
		return {};
	}

	// Global settings ...?
	//    - separate type of dataset? / special handling?
	/*
	if (!stringToArray<double>(settings.value(CameraPositionKey).toString(), m_camPosition, 3) ||
		!stringToArray<double>(settings.value(CameraFocalPointKey).toString(), m_camFocalPoint, 3) ||
		!stringToArray<double>(settings.value(CameraViewUpKey).toString(), m_camViewUp, 3))
	{
		//LOG(lvlWarn, QString("Invalid or missing camera information."));
	}
	else
	{
		m_camSettingsAvailable = true;
	}
	*/

	int maxIdx = 0;
	while (settings.contains(dataSetGroup(maxIdx) + "/File"))
	{
		++maxIdx;
	}

	int currIdx = 0;
	std::vector<std::shared_ptr<iADataSet>> dataSets;
	dataSets.reserve(maxIdx - currIdx);
	while (currIdx < maxIdx)
	{
		settings.beginGroup(dataSetGroup(currIdx));
		QString dataSetFileName = MakeAbsolute(fi.absolutePath(), settings.value("File").toString());
		try
		{
			auto io = iAFileTypeRegistry::createIO(dataSetFileName);
			if (io)
			{
				iAProgress dummyProgress;
				auto dataSetParamValues = mapFromQSettings(settings);
				auto currentLoadedDataSets = io->load(dataSetFileName, dataSetParamValues, &dummyProgress);
				for (auto dataSet : currentLoadedDataSets)
				{
					dataSets.push_back(dataSet);
				}
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
		/*
		int channel = settings.value(GetModalityKey(currIdx, "Channel"), -1).toInt();
		QString modalityRenderFlags = settings.value(GetModalityKey(currIdx, "RenderFlags")).toString();
		modalityFile = MakeAbsolute(fi.absolutePath(), modalityFile);
		QString orientationSettings = settings.value(GetModalityKey(currIdx, "Orientation")).toString();
		QString positionSettings = settings.value(GetModalityKey(currIdx, "Position")).toString();
		QString tfFileName = settings.value(GetModalityKey(currIdx, "TransferFunction")).toString();

		//loading volume settings
		iAVolumeSettings defaultSettings;
		QString Shading = settings.value(GetModalityKey(currIdx, "Shading"), defaultSettings.Shading).toString();
		QString LinearInterpolation = settings.value(GetModalityKey(currIdx, "LinearInterpolation"), defaultSettings.LinearInterpolation).toString();
		QString SampleDistance = settings.value(GetModalityKey(currIdx, "SampleDistance"), defaultSettings.SampleDistance).toString();
		QString AmbientLighting = settings.value(GetModalityKey(currIdx, "AmbientLighting"), defaultSettings.AmbientLighting).toString();
		QString DiffuseLighting = settings.value(GetModalityKey(currIdx, "DiffuseLighting"), defaultSettings.DiffuseLighting).toString();
		QString SpecularLighting = settings.value(GetModalityKey(currIdx, "SpecularLighting"), defaultSettings.SpecularLighting).toString();
		QString SpecularPower = settings.value(GetModalityKey(currIdx, "SpecularPower"), defaultSettings.SpecularPower).toString();
		QString ScalarOpacityUnitDistance = settings.value(GetModalityKey(currIdx, "ScalarOpacityUnitDistance"), defaultSettings.ScalarOpacityUnitDistance).toString();
		volSettings.RenderMode = mapRenderModeToEnum(settings.value(GetModalityKey(currIdx, "RenderMode")).toString());

		//check if vol settings are ok / otherwise use default values
		checkandSetVolumeSettings(volSettings, Shading, LinearInterpolation, SampleDistance, AmbientLighting,
			DiffuseLighting, SpecularLighting, SpecularPower, ScalarOpacityUnitDistance);

		if (!tfFileName.isEmpty())
		{
			tfFileName = MakeAbsolute(fi.absolutePath(), tfFileName);
		}
		if (modalityExists(modalityFile, channel))
		{
			LOG(lvlWarn, QString("Modality (name=%1, filename=%2, channel=%3) already exists!").arg(modalityName).arg(modalityFile).arg(channel));
		}
		else
		{
			int renderFlags = (modalityRenderFlags.contains("R") ? iAModality::MainRenderer : 0) |
				(modalityRenderFlags.contains("L") ? iAModality::MagicLens : 0) |
				(modalityRenderFlags.contains("B") ? iAModality::BoundingBox : 0) |
				(modalityRenderFlags.contains("S") ? iAModality::Slicer : 0);

			ModalityCollection mod = iAModalityList::load(modalityFile, modalityName, channel, false, renderFlags);
			if (mod.size() != 1) // we expect to load exactly one modality
			{
				LOG(lvlWarn, QString("Invalid state: More or less than one modality loaded from file '%1'").arg(modalityFile));
				return false;
			}
			mod[0]->setStringSettings(positionSettings, orientationSettings, tfFileName);
			mod[0]->setVolSettings(volSettings);
			m_modalities.push_back(mod[0]);
			emit added(mod[0]);
		}
		*/
		settings.endGroup();
		++currIdx;
		progress->emitProgress(100.0 * currIdx / maxIdx);
	}
	return dataSets;
}

QString iAProjectFileIO::name() const
{
	return Name;
}

QStringList iAProjectFileIO::extensions() const
{
	return QStringList{ "iaproj", "mod" };
}


bool iAProjectFileIO::s_bRegistered = iAFileTypeRegistry::addFileType(iAProjectFileIO::create);
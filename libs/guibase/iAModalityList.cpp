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
#include "iAModalityList.h"

#include "iALog.h"
#include "iAMathUtility.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iAProgress.h"
#include "iAXmlSettings.h"
#include "iAStringHelper.h"
#include "iAToolsVTK.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeSettings.h"
#include "iAFileUtils.h"
#include "io/iAIO.h"

#include <vtkCamera.h>
#include <vtkImageData.h>

#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>

namespace
{
	QString GetModalityKey(int idx, QString key)
	{
		return QString("Modality") + QString::number(idx) + "/" + key;
	}

	void SetBool(bool & dest, QString const & str, bool defaultVal, QString const & paramName)
	{
		QString lowerStr = str.toLower();
		if (lowerStr == "true")
			dest = true;
		else if (lowerStr == "false")
			dest = false;
		else
		{
			LOG(lvlWarn, QString("Invalid value='%1' for parameter='%2', default=%3 is applied")
				.arg(str).arg(paramName).arg(defaultVal));
			dest = defaultVal;
		}
	}

	void SetDouble(double & dest, const QString &str, double defaultVal, QString const & paramName)
	{
		bool ok = false;
		double returnVal = str.toDouble(&ok);
		if (ok)
			dest = returnVal;
		else
		{
			LOG(lvlWarn, QString("Invalid value='%1' for parameter='%2', default=%3 is applied")
				.arg(str).arg(paramName).arg(defaultVal));
			dest = defaultVal;
		}
	}

	void checkandSetVolumeSettings(iAVolumeSettings &volSettings,
		const QString & Shading, const QString & LinearInterpolation, const QString & SampleDistance,
		const QString AmbientLighting, const QString & DiffuseLighting, const QString & SpecularLighting,
		const QString & SpecularPower, const QString &ScalarOpacityUnitDistance)
	{
		iAVolumeSettings defaultSettings;
		SetBool(volSettings.Shading, Shading, defaultSettings.Shading, "Shading");
		SetBool(volSettings.LinearInterpolation, LinearInterpolation, defaultSettings.LinearInterpolation, "LinearInterpolation");
		SetDouble(volSettings.SampleDistance, SampleDistance, defaultSettings.SampleDistance, "SampleDistance");
		SetDouble(volSettings.AmbientLighting, AmbientLighting, defaultSettings.AmbientLighting, "AmbientLighting");
		SetDouble(volSettings.DiffuseLighting, DiffuseLighting, defaultSettings.DiffuseLighting, "DiffuseLighting");
		SetDouble(volSettings.SpecularLighting, SpecularLighting, defaultSettings.SpecularLighting, "SpecularLighting");
		SetDouble(volSettings.SpecularPower, SpecularPower, defaultSettings.SpecularPower, "SpecularPower");
		SetDouble(volSettings.ScalarOpacityUnitDistance, ScalarOpacityUnitDistance, defaultSettings.ScalarOpacityUnitDistance, "ScalarOpacityUnitDistance");
	}

	static const QString FileVersionKey("FileVersion");
	static const QString ModFileVersion("1.0");

	static const QString CameraPositionKey("CameraPosition");
	static const QString CameraFocalPointKey("CameraFocalPoint");
	static const QString CameraViewUpKey("CameraViewUp");
}


iAModalityList::iAModalityList() :
	m_camSettingsAvailable(false)
{
}

bool iAModalityList::modalityExists(QString const & filename, int channel) const
{
	for (QSharedPointer<iAModality> mod: m_modalities)
	{
		if (mod->fileName() == filename && mod->channel() == channel)
		{
			return true;
		}
	}
	return false;
}

QString const & iAModalityList::fileName() const
{
	return m_fileName;
}

QString GetRenderFlagString(QSharedPointer<iAModality> mod)
{
	QString result;
	if (mod->hasRenderFlag(iAModality::MagicLens)) result += "L";
	if (mod->hasRenderFlag(iAModality::MainRenderer)) result += "R";
	if (mod->hasRenderFlag(iAModality::BoundingBox)) result += "B";
	if (mod->hasRenderFlag(iAModality::Slicer)) result += "S";

	return result;
}

bool iAModalityList::store(QString const & filename, vtkCamera* camera)
{
	m_fileName = filename;
	QSettings settings(filename, QSettings::IniFormat);
	QFileInfo fi(filename);
	settings.setValue(FileVersionKey, ModFileVersion);
	if (camera)
	{
		settings.setValue(CameraPositionKey, arrayToString(camera->GetPosition(), 3));
		settings.setValue(CameraFocalPointKey, arrayToString(camera->GetFocalPoint(), 3));
		settings.setValue(CameraViewUpKey, arrayToString(camera->GetViewUp(), 3));
	}
	for (int i = 0; i< m_modalities.size(); ++i)
	{
		QFileInfo modalityFileInfo(m_modalities[i]->fileName());
		if (!modalityFileInfo.exists() || !modalityFileInfo.isFile())
		{	// TODO: provide option to store as .mhd?
			QMessageBox::warning(nullptr, "Save Project",
				QString("Cannot reference %1 in project. Maybe this is an image stack? Please store modality first as file.").arg(m_modalities[i]->fileName()));
			if (fi.exists())
			{
				// remove any half-written project file
				QFile::remove(fi.absoluteFilePath());
			}
			return false;
		}
		settings.setValue(GetModalityKey(i, "Name"), m_modalities[i]->name());
		settings.setValue(GetModalityKey(i, "File"), MakeRelative(fi.absolutePath(), m_modalities[i]->fileName()));
		if (m_modalities[i]->channel() >= 0)
		{
			settings.setValue(GetModalityKey(i, "Channel"), m_modalities[i]->channel());
		}
		settings.setValue(GetModalityKey(i, "RenderFlags"), GetRenderFlagString(m_modalities[i]));
		settings.setValue(GetModalityKey(i, "Orientation"), m_modalities[i]->orientationString());
		settings.setValue(GetModalityKey(i, "Position"), m_modalities[i]->positionString());

		//save renderer volume settings for each modality
		iAVolumeSettings const & volumeSettings = m_modalities[i]->renderer()->volumeSettings();
		settings.setValue(GetModalityKey(i, "Shading"), volumeSettings.Shading);
		settings.setValue(GetModalityKey(i, "LinearInterpolation"), volumeSettings.LinearInterpolation);
		settings.setValue(GetModalityKey(i, "SampleDistance"), volumeSettings.SampleDistance);
		settings.setValue(GetModalityKey(i, "AmbientLighting"), volumeSettings.AmbientLighting);
		settings.setValue(GetModalityKey(i, "DiffuseLighting"), volumeSettings.DiffuseLighting);
		settings.setValue(GetModalityKey(i, "SpecularLighting"), volumeSettings.SpecularLighting);
		settings.setValue(GetModalityKey(i, "SpecularPower"), volumeSettings.SpecularPower);
		settings.setValue(GetModalityKey(i, "ScalarOpacityUnitDistance"), volumeSettings.ScalarOpacityUnitDistance);
		settings.setValue(GetModalityKey(i, "RenderMode"), volumeSettings.RenderMode);

		QFileInfo modFileInfo(m_modalities[i]->fileName());
		QString absoluteTFFileName(m_modalities[i]->transferFileName());
		if (absoluteTFFileName.isEmpty())
		{
			absoluteTFFileName = MakeAbsolute(fi.absolutePath(), modFileInfo.fileName() + "_tf.xml");
			QFileInfo uniqueNameSearchFileInfo(absoluteTFFileName);
			int suffix = 1;
			while (uniqueNameSearchFileInfo.exists())
			{
				absoluteTFFileName = MakeAbsolute(uniqueNameSearchFileInfo.absolutePath(), modFileInfo.fileName() + "_tf-" + QString::number(suffix) + ".xml");
				uniqueNameSearchFileInfo.setFile(absoluteTFFileName);
				++suffix;
			}
		}
		QString tfFileName = MakeRelative(fi.absolutePath(), absoluteTFFileName);
		settings.setValue(GetModalityKey(i, "TransferFunction"), tfFileName);
		iAXmlSettings s;
		s.saveTransferFunction(m_modalities[i]->transfer().data());
		s.save(absoluteTFFileName);
	}
	return true;
}

bool iAModalityList::load(QString const & filename, iAProgress& progress)
{
	if (filename.isEmpty())
	{
		LOG(lvlError, "No project file given.");
		return false;
	}
	QFileInfo fi(filename);
	if (!fi.exists())
	{
		LOG(lvlError, QString("Given project file '%1' does not exist.").arg(filename));
		return false;
	}
	QSettings settings(filename, QSettings::IniFormat);
	iAVolumeSettings volSettings;

	if (!settings.contains(FileVersionKey) ||
		settings.value(FileVersionKey).toString() != ModFileVersion)
	{
		LOG(lvlError, QString("Invalid project file version (was %1, expected %2)! Trying to parse anyway, but expect failures.")
			.arg(settings.contains(FileVersionKey) ? settings.value(FileVersionKey).toString() : "not set")
			.arg(ModFileVersion));
		return false;
	}
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

	int maxIdx = 0;
	while (settings.contains(GetModalityKey(maxIdx, "Name")))
	{
		++maxIdx;
	}

	int currIdx = 0;
	while (currIdx < maxIdx)
	{
		QString modalityName = settings.value(GetModalityKey(currIdx, "Name")).toString();
		QString modalityFile = settings.value(GetModalityKey(currIdx, "File")).toString();
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
		++currIdx;
		progress.emitProgress(100.0 * currIdx / maxIdx);
	}
	m_fileName = filename;
	return true;
}

void iAModalityList::applyCameraSettings(vtkCamera* camera)
{
	if (!camera || !m_camSettingsAvailable)
	{
		return;
	}
	camera->SetPosition(m_camPosition);
	camera->SetFocalPoint(m_camFocalPoint);
	camera->SetViewUp(m_camViewUp);
	m_camSettingsAvailable = false;
}

/*
namespace
{
	QString GetMeasurementString(QSharedPointer<iAModality> mod)
	{
		return QString("") + QString::number(mod->width()) + "x" +
			QString::number(mod->height()) + "x" +
			QString::number(mod->depth()) + " (" +
			QString::number(mod->spacing()[0]) + ", " +
			QString::number(mod->spacing()[1]) + ", " +
			QString::number(mod->spacing()[2]) + ")";
	}
}
*/

void iAModalityList::add(QSharedPointer<iAModality> mod)
{
	if (m_modalities.size() > 0)
	{
		// make sure that size & spacing fit:
		/*
		if (m_modalities[0]->GetWidth() != mod->GetWidth() ||
		m_modalities[0]->GetHeight() != mod->GetHeight() ||
		m_modalities[0]->GetDepth() != mod->GetDepth() ||
		m_modalities[0]->GetSpacing()[0] != mod->GetSpacing()[0] ||
		m_modalities[0]->GetSpacing()[1] != mod->GetSpacing()[1] ||
		m_modalities[0]->GetSpacing()[2] != mod->GetSpacing()[2])
		{
		DebugOut() << "Measurements of new modality " <<
		GetMeasurementString(mod) << " don't fit measurements of existing one: " <<
		GetMeasurementString(m_modalities[0]) << std::endl;
		return;
		}
		*/
	}
	m_modalities.push_back(mod);
	emit added(mod);
}

void iAModalityList::remove(int idx)
{
	m_modalities.remove(idx);
}

QSharedPointer<iAModality> iAModalityList::get(int idx)
{
	return m_modalities[idx];
}

QSharedPointer<iAModality const> iAModalityList::get(int idx) const
{
	return m_modalities[idx];
}

int iAModalityList::size() const
{
	return m_modalities.size();
}

ModalityCollection iAModalityList::load(QString const & filename, QString const & name, int channel, bool split, int renderFlags)
{
	// TODO: unify this with mdichild::loadFile
	ModalityCollection result;
	if (!QFileInfo(filename).exists())
	{
		LOG(lvlError, QString("File %1 does not exist!").arg(filename));
		return result;
	}
	QFileInfo fileInfo(filename);
	vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();
	std::vector<vtkSmartPointer<vtkImageData> > volumes;
	iAIO io(img, 0, 0, 0, &volumes);
	if (filename.endsWith(iAIO::VolstackExtension))
	{
		io.setupIO(VOLUME_STACK_VOLSTACK_READER, filename);
	}
	else
	{
		QString extension = fileInfo.suffix();
		extension = extension.toUpper();
		mapQString2int const & ext2id = extensionToId();
		if (ext2id.find(extension) == ext2id.end())
		{
			LOG(lvlError, "Unknown file type!");
			return result;
		}
		iAIOType id = ext2id.find(extension).value();
		if (!io.setupIO(id, filename, false, channel))
		{
			LOG(lvlError, "Could not set up modality loading!");
			return result;
		}
	}
	io.start();
	io.wait();
	QString nameBase = name.isEmpty() ? fileInfo.baseName() : name;
	if (volumes.size() > 1 && (channel < 0 || static_cast<size_t>(channel) > volumes.size()))
	{
		if (split) // load one modality for each channel
		{
			for (size_t i = 0; i < volumes.size(); ++i)
			{
				QSharedPointer<iAModality> newModality(new iAModality(
					QString("%1-%2").arg(nameBase).arg(i),
					filename, static_cast<int>(i), volumes[i], renderFlags));		// TODO: use different renderFlag for first channel?
				result.push_back(newModality);
			}
		}
		else       // load modality with multiple components
		{
			QSharedPointer<iAModality> newModality(new iAModality(
				nameBase, filename, volumes, renderFlags));
			result.push_back(newModality);
		}
	}
	else           // load single modality
	{
		if (volumes.size() > 0)
		{
			channel = clamp(0, static_cast<int>(volumes.size() - 1), channel);
			img = volumes[channel];
		}
		if (!img || img->GetDimensions()[0] == 0 || img->GetDimensions()[1] == 0)
		{
			LOG(lvlError, QString("File '%1' could not be loaded!").arg(filename));
			return result;
		}
		QSharedPointer<iAModality> newModality(new iAModality(
			nameBase, filename, channel, img, renderFlags));
		result.push_back(newModality);
	}
	return result;
}


bool iAModalityList::hasUnsavedModality() const
{
	for (int i = 0; i < m_modalities.size(); ++i)
	{
		if (m_modalities[i]->fileName().isEmpty() || !QFileInfo(m_modalities[i]->fileName()).exists())
		{
			return true;
		}
	}
	return false;
}

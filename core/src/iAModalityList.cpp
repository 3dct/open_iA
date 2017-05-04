/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "extension2id.h"
#include "iAConsole.h"
#include "iAFileUtils.h"
#include "iAIO.h"
#include "iAMathUtility.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iASettings.h"
#include "iAStringHelper.h"

#include <vtkCamera.h>
#include <vtkImageData.h>

#include <QFileInfo>
#include <QSettings>

namespace
{
	QString GetModalityKey(int idx, QString key)
	{
		return QString("Modality") + QString::number(idx) + "/" + key;
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

bool iAModalityList::ModalityExists(QString const & filename, int channel) const
{
	foreach(QSharedPointer<iAModality> mod, m_modalities)
	{
		if (mod->GetFileName() == filename && mod->GetChannel() == channel)
		{
			return true;
		}
	}
	return false;
}

QString const & iAModalityList::GetFileName() const
{
	return m_fileName;
}

QString GetRenderFlagString(QSharedPointer<iAModality> mod)
{
	QString result;
	if (mod->hasRenderFlag(iAModality::MagicLens)) result += "L";
	if (mod->hasRenderFlag(iAModality::MainRenderer)) result += "R";
	if (mod->hasRenderFlag(iAModality::BoundingBox)) result += "B";
	return result;
}

void iAModalityList::Store(QString const & filename, vtkCamera* camera)
{
	m_fileName = filename;
	QSettings settings(filename, QSettings::IniFormat);
	QFileInfo fi(filename);
	settings.setValue(FileVersionKey, ModFileVersion);
	if (camera)
	{
		settings.setValue(CameraPositionKey, Vec3D2String(camera->GetPosition()));
		settings.setValue(CameraFocalPointKey, Vec3D2String(camera->GetFocalPoint()));
		settings.setValue(CameraViewUpKey, Vec3D2String(camera->GetViewUp()));
	}
	for (int i = 0; i<m_modalities.size(); ++i)
	{
		settings.setValue(GetModalityKey(i, "Name"), m_modalities[i]->GetName());
		settings.setValue(GetModalityKey(i, "File"), MakeRelative(fi.absolutePath(), m_modalities[i]->GetFileName()));
		if (m_modalities[i]->GetChannel() >= 0)
		{
			settings.setValue(GetModalityKey(i, "Channel"), m_modalities[i]->GetChannel());
		}
		settings.setValue(GetModalityKey(i, "RenderFlags"), GetRenderFlagString(m_modalities[i]));
		settings.setValue(GetModalityKey(i, "Orientation"), m_modalities[i]->GetOrientationString());
		settings.setValue(GetModalityKey(i, "Position"), m_modalities[i]->GetPositionString());
		QFileInfo modFileInfo(m_modalities[i]->GetFileName());
		QString absoluteTFFileName(m_modalities[i]->GetTransferFileName());
		if (absoluteTFFileName.isEmpty())
		{
			absoluteTFFileName = MakeAbsolute(fi.absolutePath(), modFileInfo.fileName() + "_tf.xml");
			QFileInfo fi(absoluteTFFileName);
			int i = 1;
			while (fi.exists())
			{
				absoluteTFFileName = MakeAbsolute(fi.absolutePath(), modFileInfo.fileName() + "_tf-" + QString::number(i) + ".xml");
				fi.setFile(absoluteTFFileName);
				++i;
			}
		}
		QString tfFileName = MakeRelative(fi.absolutePath(), absoluteTFFileName);
		settings.setValue(GetModalityKey(i, "TransferFunction"), tfFileName);
		Settings s;
		s.StoreTransferFunction(m_modalities[i]->GetTransfer().data());
		s.Save(absoluteTFFileName);
	}
}

bool iAModalityList::Load(QString const & filename)
{
	if (filename.isEmpty())
	{
		DEBUG_LOG("No modality file given.");
		return false;
	}
	QFileInfo fi(filename);
	if (!fi.exists())
	{
		DEBUG_LOG(QString("Given modality file '%1' does not exist.").arg(filename));
		return false;
	}
	QSettings settings(filename, QSettings::IniFormat);

	if (!settings.contains(FileVersionKey) ||
		settings.value(FileVersionKey).toString() != ModFileVersion)
	{
		DEBUG_LOG(QString("Invalid modality file version (was %1, expected %2! Trying to parse anyway, but expect failures.")
			.arg(settings.contains(FileVersionKey) ? settings.value(FileVersionKey).toString() : "not set")
			.arg(ModFileVersion));
		return false;
	}
	if (!Str2Vec3D(settings.value(CameraPositionKey).toString(), camPosition) ||
		!Str2Vec3D(settings.value(CameraFocalPointKey).toString(), camFocalPoint) ||
		!Str2Vec3D(settings.value(CameraViewUpKey).toString(), camViewUp))
	{
		//DEBUG_LOG(QString("Invalid or missing camera information."));
	}
	else
	{
		m_camSettingsAvailable = true;
	}

	int currIdx = 0;

	while (settings.contains(GetModalityKey(currIdx, "Name")))
	{
		QString modalityName = settings.value(GetModalityKey(currIdx, "Name")).toString();
		QString modalityFile = settings.value(GetModalityKey(currIdx, "File")).toString();
		int channel = settings.value(GetModalityKey(currIdx, "Channel"), -1).toInt();
		QString modalityRenderFlags = settings.value(GetModalityKey(currIdx, "RenderFlags")).toString();
		modalityFile = MakeAbsolute(fi.absolutePath(), modalityFile);
		QString orientationSettings = settings.value(GetModalityKey(currIdx, "Orientation")).toString();
		QString positionSettings = settings.value(GetModalityKey(currIdx, "Position")).toString();
		QString tfFileName = settings.value(GetModalityKey(currIdx, "TransferFunction")).toString();
		if (!tfFileName.isEmpty())
		{
			tfFileName = MakeAbsolute(fi.absolutePath(), tfFileName);
		}
		if (ModalityExists(modalityFile, channel))
		{
			DEBUG_LOG(QString("Modality (name=%1, filename=%2, channel=%3) already exists!").arg(modalityName).arg(modalityFile).arg(channel));
		}
		else
		{
			int renderFlags = (modalityRenderFlags.contains("R") ? iAModality::MainRenderer : 0) |
				(modalityRenderFlags.contains("L") ? iAModality::MagicLens : 0) |
				(modalityRenderFlags.contains("B") ? iAModality::BoundingBox : 0);

			ModalityCollection mod = iAModalityList::Load(modalityFile, modalityName, channel, false, renderFlags);
			if (mod.size() != 1) // we expect to load exactly one modality
			{
				DEBUG_LOG(QString("Invalid state: More or less than one modality loaded from file '%1'").arg(modalityFile));
				return false;
			}
			mod[0]->SetStringSettings(positionSettings, orientationSettings, tfFileName);
			m_modalities.push_back(mod[0]);
			emit Added(mod[0]);
		}
		currIdx++;
	}
	m_fileName = filename;
	return true;
}

void iAModalityList::ApplyCameraSettings(vtkCamera* camera)
{
	if (!camera || !m_camSettingsAvailable)
	{
		return;
	}
	camera->SetPosition(camPosition);
	camera->SetFocalPoint(camFocalPoint);
	camera->SetViewUp(camViewUp);
}

namespace
{
	QString GetMeasurementString(QSharedPointer<iAModality> mod)
	{
		return QString("") + QString::number(mod->GetWidth()) + "x" +
			QString::number(mod->GetHeight()) + "x" +
			QString::number(mod->GetDepth()) + " (" +
			QString::number(mod->GetSpacing()[0]) + ", " +
			QString::number(mod->GetSpacing()[1]) + ", " +
			QString::number(mod->GetSpacing()[2]) + ")";
	}
}

void iAModalityList::Add(QSharedPointer<iAModality> mod)
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
	emit Added(mod);
}

void iAModalityList::Remove(int idx)
{
	m_modalities.remove(idx);
}

QSharedPointer<iAModality> iAModalityList::Get(int idx)
{
	return m_modalities[idx];
}

QSharedPointer<iAModality const> iAModalityList::Get(int idx) const
{
	return m_modalities[idx];
}

int iAModalityList::size() const
{
	return m_modalities.size();
}

ModalityCollection iAModalityList::Load(QString const & filename, QString const & name, int channel, bool split, int renderFlags)
{
	// TODO: unify this with mdichild::loadFile
	ModalityCollection result;
	QFileInfo fileInfo(filename);
	vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();
	std::vector<vtkSmartPointer<vtkImageData> > volumes;
	iAIO io(img, 0, 0, 0, &volumes);
	if (filename.endsWith(iAIO::VolstackExtension))
	{
		io.setupIO(VOLUME_STACK_VOLSTACK_READER, filename.toLatin1().data());
	}
	else
	{
		QString extension = fileInfo.suffix();
		extension = extension.toUpper();
		const mapQString2int * ext2id = &extensionToId;
		if (ext2id->find(extension) == ext2id->end())
		{
			DEBUG_LOG("Unknown file type!");
			return result;
		}
		IOType id = ext2id->find(extension).value();
		if (!io.setupIO(id, filename, false, channel))
		{
			DEBUG_LOG("Error while setting up modality loading!");
			return result;
		}
	}
	io.start();
	io.wait();
	QString nameBase = name.isEmpty() ? fileInfo.baseName() : name;
	if (volumes.size() > 1 && (channel < 0 || channel > volumes.size()))
	{
		if (split) // load one modality for each channel
		{
			int channels = volumes.size();
			for (int i = 0; i < channels; ++i)
			{
				QSharedPointer<iAModality> newModality(new iAModality(
					QString("%1-%2").arg(nameBase).arg(i),
					filename, i, volumes[i], renderFlags));		// TODO: use different renderFlag for first channel?
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
			DEBUG_LOG(QString("File '%1' could not be loaded!").arg(filename));
			return result;
		}
		QSharedPointer<iAModality> newModality(new iAModality(
			nameBase, filename, channel, img, renderFlags));
		result.push_back(newModality);
	}
	return result;
}

/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "pch.h"
#include "iAModality.h"

#include "iAConsole.h"
#include "iAIO.h"
#include "iAImageCoordinate.h"
#include "iAVolumeRenderer.h"
#include "iAModalityTransfer.h"
#include "iASettings.h"
#include "extension2id.h"

#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkVolume.h>

#include <QFileInfo>
#include <QSettings>

#include <cassert>


iAModality::iAModality(QString const & name, QString const & filename, int channel, int renderFlags):
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(channel)
{
}

iAModality::iAModality(QString const & name, QString const & filename, int channel, vtkSmartPointer<vtkImageData> imgData, int renderFlags) :
	m_name(name),
	m_filename(filename),
	renderFlags(renderFlags),
	m_channel(channel)
{
	SetData(imgData);
}

QString iAModality::GetName() const
{
	return m_name;
}

QString iAModality::GetFileName() const
{
	return m_filename;
}

int iAModality::GetChannel() const
{
	return m_channel;
}

void iAModality::SetName(QString const & name)
{
	m_name = name;
}

void iAModality::SetRenderFlag(int renderFlags)
{
	this->renderFlags = renderFlags;
}

int iAModality::GetWidth() const
{
	assert(m_converter);
	return m_converter->GetWidth();
}

int iAModality::GetHeight() const
{
	assert(m_converter);
	return m_converter->GetHeight();
}

int iAModality::GetDepth() const
{
	assert(m_converter);
	return m_converter->GetDepth();
}

double const * iAModality::GetSpacing() const
{
	return m_spacing;
}

iAImageCoordConverter const & iAModality::GetConverter() const
{
	assert(m_converter);
	return *m_converter;
}

vtkSmartPointer<vtkImageData> iAModality::GetImage() const
{
	return m_imgData;
}

bool iAModality::hasRenderFlag(RenderFlag loc) const
{
	return (renderFlags & loc) == loc;
}


int iAModality::RenderFlags() const
{
	return renderFlags;
}

void iAModality::SetTransfer(QSharedPointer<iAModalityTransfer> transfer)
{
	// TODO: VOLUME: rewrite / move to iAModalityTransfer constructor if possible!
	m_transfer = transfer;
}


void iAModality::LoadTransferFunction()
{
	if (tfFileName.isEmpty())
	{
		return;
	}
	Settings s(tfFileName);
	s.LoadTransferFunction(GetTransfer().data(), GetImage()->GetScalarRange());
	tfFileName = "";
}

QSharedPointer<iAModalityTransfer> iAModality::GetTransfer()
{
	return m_transfer;
}

bool Str2Vec3D(QString const & str, double vec[3])
{
	QStringList list = str.split(" ");
	if (list.size() != 3)
	{
		return false;
	}
	for (int i = 0; i < 3; ++i)
	{
		bool ok;
		vec[i] = list[i].toDouble(&ok);
		if (!ok)
			return false;
	}
	return true;
}

void iAModality::SetRenderer(QSharedPointer<iAVolumeRenderer> renderer)
{
	m_renderer = renderer;
	if (orientationSettings.isEmpty() || positionSettings.isEmpty())
	{
		return;
	}
	double position[3];
	double orientation[3];
	if (!Str2Vec3D(orientationSettings, orientation) ||
		!Str2Vec3D(positionSettings, position))
	{
		//DEBUG_LOG("Invalid orientation/position!");
		return;
	}
	renderer->SetPosition(position);
	renderer->SetOrientation(orientation);
}

QSharedPointer<iAVolumeRenderer> iAModality::GetRenderer()
{
	return m_renderer;
}

void iAModality::InitHistogram()
{
	GetTransfer()->InitHistogram(GetImage());
	LoadTransferFunction();
}

bool iAModality::LoadData()
{
	// TODO: unify this with mdichild::loadFile / dlg_modalities::AddClicked!
	if (m_filename.endsWith(iAIO::VolstackExtension))
	{
		std::vector<vtkSmartPointer<vtkImageData> > volumes;
		iAIO io(
			0,
			0,
			&volumes
		);
		io.setupIO(VOLUME_STACK_VOLSTACK_READER, m_filename.toLatin1().data());
		io.start();
		io.wait();
		if (volumes.size() == 0)
		{
			DEBUG_LOG("No volume found in stack!");
			return false;
		}
		if (m_channel < 0 || m_channel > volumes.size())
		{
			DEBUG_LOG(QString("Channel number %1 outside of valid interval [0..%2]").arg(m_channel).arg(volumes.size()));
			m_channel = 0;
		}
		SetData(volumes[m_channel]);
	}
	else
	{
		vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();
		std::vector<vtkSmartPointer<vtkImageData> > volumes;
		iAIO io(img, 0, 0);

		QFileInfo fileInfo(m_filename);
		QString extension = fileInfo.suffix();
		extension = extension.toUpper();
		const mapQString2int * ext2id = &extensionToId;
		if (ext2id->find(extension) == ext2id->end())
		{
			DEBUG_LOG("Unknown file type!");
			return false;
		}
		IOType id = ext2id->find(extension).value();
		if (m_channel < 0 || m_channel > volumes.size())
		{
			DEBUG_LOG(QString("Channel number %1 outside of valid interval [0..%2]").arg(m_channel).arg(volumes.size() - 1));
			m_channel = 0;
		}
		if (!io.setupIO(id, m_filename, false, m_channel))
		{
			DEBUG_LOG("Error while setting up modality loading!");
			return false;
		}
		// TODO: check for errors during actual loading!
		//connect(io, done(bool), this, )
		io.start();
		// TODO: VOLUME: make asynchronous!
		io.wait();
		SetData(img);
	}
	return true;
}

void iAModality::SetData(vtkSmartPointer<vtkImageData> imgData)
{
	assert(imgData);
	m_imgData = imgData;
	int extent[6];
	imgData->GetExtent(extent);
	m_converter = QSharedPointer<iAImageCoordConverter>(new iAImageCoordConverter(
		extent[1] - extent[0] + 1, extent[3] - extent[2] + 1, extent[5] - extent[4] + 1));
	imgData->GetSpacing(m_spacing);
}


// iAModalityList
#include "iAFileUtils.h"

namespace
{
	QString GetModalityKey(int idx, QString key)
	{
		return QString("Modality")+QString::number(idx)+"/"+key;
	}

	static const QString FileVersionKey("FileVersion");
	static const QString ModFileVersion("1.0");
	static const QString SetSpacingToOneKey("SetSpacingToOne");

	static const QString CameraPositionKey("CameraPosition");
	static const QString CameraFocalPointKey("CameraFocalPoint");
	static const QString CameraViewUpKey("CameraViewUp");
}


iAModalityList::iAModalityList():
	m_camSettingsAvailable(false)
{
	m_spacing[0] = m_spacing[1] = m_spacing[2] = 1.0;
}

bool iAModalityList::ModalityExists(QString const & filename) const
{
	foreach (QSharedPointer<iAModality> mod, m_modalities)
	{
		if (mod->GetFileName() == filename)
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

QString Vec3D2String(double* vec)
{
	return QString("%1 %2 %3").arg(vec[0]).arg(vec[1]).arg(vec[2]);
}

QString GetOrientation(QSharedPointer<iAVolumeRenderer> renderer)
{
	double * orientation = renderer->GetOrientation();
	return Vec3D2String(orientation);
}

QString GetPosition(QSharedPointer<iAVolumeRenderer> renderer)
{
	double * position = renderer->GetPosition();
	return Vec3D2String(position);
}

void iAModalityList::Store(QString const & filename, vtkCamera* camera)
{
	m_fileName = filename;
	QSettings settings(filename, QSettings::IniFormat );
	QFileInfo fi(filename);
	settings.setValue(FileVersionKey, ModFileVersion);
	if (camera)
	{
		settings.setValue(CameraPositionKey, Vec3D2String(camera->GetPosition()));
		settings.setValue(CameraFocalPointKey, Vec3D2String(camera->GetFocalPoint()));
		settings.setValue(CameraViewUpKey, Vec3D2String(camera->GetViewUp()));
	}
	for (int i=0; i<m_modalities.size(); ++i)
	{
		settings.setValue(GetModalityKey(i, "Name"), m_modalities[i]->GetName());
		if (m_modalities[i]->GetChannel() >= 0)
		{
			settings.setValue(GetModalityKey(i, "Channel"), m_modalities[i]->GetChannel());
		}
		settings.setValue(GetModalityKey(i, "File"), MakeRelative(fi.absolutePath(), m_modalities[i]->GetFileName()));
		settings.setValue(GetModalityKey(i, "RenderFlags"), GetRenderFlagString(m_modalities[i]) );
		settings.setValue(GetModalityKey(i, "Orientation"), GetOrientation(m_modalities[i]->GetRenderer()));
		settings.setValue(GetModalityKey(i, "Position"), GetPosition(m_modalities[i]->GetRenderer()));
		QFileInfo modFileInfo(m_modalities[i]->GetFileName());
		//m_modalities[i]->GetTransferFileName();
		QString absoluteTFFileName(modFileInfo.absoluteFilePath() + "_tf.xml");
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
	QSettings settings(filename, QSettings::IniFormat );
	
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

	bool setSpacingToOne = settings.contains(SetSpacingToOneKey) && settings.value(SetSpacingToOneKey).toBool();
	int currIdx = 0;
	
	double spacingFactor[3] = { 1.0, 1.0, 1.0 };
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
		tfFileName = MakeAbsolute(fi.absolutePath(), tfFileName);
		if (ModalityExists(modalityFile))
		{
			//DebugOut () << "Modality (name="<<modalityName<<", filename="<<modalityFile<<") already exists!" << std::endl;
		}
		else
		{
			int renderFlags = (modalityRenderFlags.isEmpty() || modalityRenderFlags.contains("R") ? iAModality::MainRenderer : 0) |
				(modalityRenderFlags.contains("L") ? iAModality::MagicLens : 0) |
				(modalityRenderFlags.contains("B") ? iAModality::BoundingBox : 0);

			QSharedPointer<iAModality> mod(new iAModality(modalityName, modalityFile, channel, renderFlags));
			if (!mod->LoadData())
			{
				return false;
			}

			// fake a spacing of 1 1 1 for main dataset (to improve transparency renderings)
			if (setSpacingToOne)
			{
				if (currIdx == 0)
				{
					mod->GetImage()->GetSpacing(spacingFactor);
				}
				double spacing[3];
				mod->GetImage()->GetSpacing(spacing);
				mod->GetImage()->SetSpacing(spacing[0] / spacingFactor[0], spacing[1] / spacingFactor[1], spacing[2] / spacingFactor[2]);
			}

			mod->orientationSettings = orientationSettings;
			mod->positionSettings = positionSettings;
			mod->tfFileName = tfFileName;

			m_modalities.push_back(mod);
			emit Added(mod);
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

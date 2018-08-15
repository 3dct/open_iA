/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAConsole.h"
#include "iAMathUtility.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "iASettings.h"
#include "iAStringHelper.h"
#include "iAVolumeRenderer.h"
#include "iAVolumeSettings.h"
#include "io/extension2id.h"
#include "io/iAFileUtils.h"
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

	//returns -1 if String is not true or false;
	int isStringBoolean(const QString &str, bool &booleanStr) {
		int isBoolean = 0; 
		QString tmp_str = str.toLower();
		if (tmp_str.compare("true") == 0) {
			booleanStr = true; 
			isBoolean = 1; 
			 
		}
		else if (tmp_str.compare("false") == 0) {
			booleanStr = false;
			isBoolean = 1;
		}
		else isBoolean =-1;

		return isBoolean; 

		
	}

	int isStringDouble(const QString &str, double &stringValue) {
		bool  ok=false; 
		stringValue = str.toDouble(&ok);
		

		if (ok) return 1;
		else return -1; 
	}


	void logParameter(const QString &para_name, const QString &str_in ) {
		DEBUG_LOG(QString("invalid parameter" +para_name+ "default value is applied").arg(str_in));
	
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
	foreach(QSharedPointer<iAModality> mod, m_modalitiesActive)
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
	for (int i = 0; i<m_modalitiesActive.size(); ++i)
	{
		QFileInfo modalityFileInfo(m_modalitiesActive[i]->GetFileName());
		if (!modalityFileInfo.exists() || !modalityFileInfo.isFile())
		{	// TODO: provide option to store as .mhd?
			QMessageBox::warning(nullptr, "Save Project",
				QString("Cannot reference %1 in project. Maybe this is an image stack? Please store modality first as file.").arg(m_modalitiesActive[i]->GetFileName()));
			if (fi.exists())
			{
				// remove any half-written project file
				QFile::remove(fi.absoluteFilePath());
			}
			return;
		}
		settings.setValue(GetModalityKey(i, "Name"), m_modalitiesActive[i]->GetName());
		settings.setValue(GetModalityKey(i, "File"), MakeRelative(fi.absolutePath(), m_modalitiesActive[i]->GetFileName()));
		if (m_modalitiesActive[i]->GetChannel() >= 0)
		{
			settings.setValue(GetModalityKey(i, "Channel"), m_modalitiesActive[i]->GetChannel());
		}
		settings.setValue(GetModalityKey(i, "RenderFlags"), GetRenderFlagString(m_modalitiesActive[i]));
		settings.setValue(GetModalityKey(i, "Orientation"), m_modalitiesActive[i]->GetOrientationString());
		settings.setValue(GetModalityKey(i, "Position"), m_modalitiesActive[i]->GetPositionString());
		
		//save renderer volume settings for each modality
		settings.setValue(GetModalityKey(i, "Shading"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().Shading);
		
		settings.setValue(GetModalityKey(i, "LinearInterpolation"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().LinearInterpolation);
		settings.setValue(GetModalityKey(i, "SampleDistance"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().SampleDistance);
		settings.setValue(GetModalityKey(i, "AmbientLighting"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().AmbientLighting);
		settings.setValue(GetModalityKey(i, "DiffuseLighting"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().DiffuseLighting);
		settings.setValue(GetModalityKey(i, "SpecularLighting"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().SpecularLighting);
		settings.setValue(GetModalityKey(i, "SpecularPower"), m_modalitiesActive[i]->GetRenderer()->getVolumeSettings().SpecularPower);



		QFileInfo modFileInfo(m_modalitiesActive[i]->GetFileName());
		QString absoluteTFFileName(m_modalitiesActive[i]->GetTransferFileName());
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
		iASettings s;
		s.StoreTransferFunction(m_modalitiesActive[i]->GetTransfer().data());
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
	iAVolumeSettings volSettings; 

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


		//loading volume settings
		QString Shading = settings.value(GetModalityKey(currIdx, "Shading")).toString();
		QString LinearInterpolation = settings.value(GetModalityKey(currIdx, "LinearInterpolation")).toString();
		QString SampleDistance = settings.value(GetModalityKey(currIdx, "SampleDistance")).toString();
		QString AmbientLighting = settings.value(GetModalityKey(currIdx, "AmbientLighting")).toString();
		QString DiffuseLighting = settings.value(GetModalityKey(currIdx, "DiffuseLighting")).toString();
		QString SpecularLighting = settings.value(GetModalityKey(currIdx, "SpecularLighting")).toString();
		QString SpecularPower = settings.value(GetModalityKey(currIdx, "SpecularPower")).toString();


		//check if vol settings are ok / otherwise use default values 
		checkandSetVolumeSettings(volSettings, Shading, LinearInterpolation, SampleDistance, AmbientLighting,
			DiffuseLighting, SpecularLighting, SpecularPower);
		//Werte laden

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
			
			//volume settings added to each modality
			mod[0]->setVolSettings(volSettings);
			

			m_modalitiesActive.push_back(mod[0]);
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
	m_camSettingsAvailable = false;
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
	if (m_modalitiesActive.size() > 0)
	{
		// make sure that size & spacing fit:
		/*
		if (m_modalitiesActive[0]->GetWidth() != mod->GetWidth() ||
		m_modalitiesActive[0]->GetHeight() != mod->GetHeight() ||
		m_modalitiesActive[0]->GetDepth() != mod->GetDepth() ||
		m_modalitiesActive[0]->GetSpacing()[0] != mod->GetSpacing()[0] ||
		m_modalitiesActive[0]->GetSpacing()[1] != mod->GetSpacing()[1] ||
		m_modalitiesActive[0]->GetSpacing()[2] != mod->GetSpacing()[2])
		{
		DebugOut() << "Measurements of new modality " <<
		GetMeasurementString(mod) << " don't fit measurements of existing one: " <<
		GetMeasurementString(m_modalitiesActive[0]) << std::endl;
		return;
		}
		*/
	}
	m_modalitiesActive.push_back(mod);
	emit Added(mod);
}

void iAModalityList::Remove(int idx)
{
	m_modalitiesActive.remove(idx);
}

QSharedPointer<iAModality> iAModalityList::Get(int idx)
{
	return m_modalitiesActive[idx];
}

QSharedPointer<iAModality const> iAModalityList::Get(int idx) const
{
	return m_modalitiesActive[idx];
}

int iAModalityList::size() const
{
	return m_modalitiesActive.size();
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


bool iAModalityList::HasUnsavedModality() const
{
	for (int i = 0; i < m_modalitiesActive.size(); ++i)
	{
		if (m_modalitiesActive[i]->GetFileName().isEmpty() || !QFileInfo(m_modalitiesActive[i]->GetFileName()).exists())
		{
			return true;
		}
	}
	return false;
}

void iAModalityList::checkandSetVolumeSettings(iAVolumeSettings &volSettings, const QString & Shading, const QString & LinearInterpolation, const QString & SampleDistance, 
	const QString AmbientLighting, const QString & DiffuseLighting, const QString & SpecularLighting, const QString & SpecularPower)
{
	bool volumeSettingsTrue = true; 
	bool b_Shading = false; 
	bool b_LinearInterPol = false; 
	double d_sampleDistance = 0.0;
	double d_ambientLighting = 0.0;
	double d_diffuseLighting = 0.0; 
	double d_SpecularLighting = 0.0; 
	double d_SpecularPower = 0.0; 


	//default values; 
	const bool b_DefaultLinearInterPol = true; 
	const bool b_DefaultShading = true; 
	const double d_DefaultSampleDistance  = 2;
	const double d_DefaultAmbientLight = 0.2;
	const double d_DefaultDiffuseLight = 0.5;
	const double d_DefaultSpecularLighting = 0.7;
	const double d_DefaultSpecularPower = 10; 


	//Shading
	if(isStringBoolean(Shading, b_Shading)){
		volSettings.Shading = b_Shading; 
	}else {
		logParameter("Shading", Shading); 
		volSettings.Shading =true; 
	}

	

	//LinearInterpolation
	if (isStringBoolean(LinearInterpolation, b_LinearInterPol)){
		volSettings.LinearInterpolation = b_LinearInterPol; 
	}
	else {
		logParameter("LinearInterpolation", LinearInterpolation);
		volSettings.LinearInterpolation = b_DefaultLinearInterPol; 
	}

	

	//SampleDistance
	if (isStringDouble(SampleDistance, d_sampleDistance)) {
		volSettings.SampleDistance = d_sampleDistance;
	
	}else {
		logParameter("SampleDistance", SampleDistance);
		volSettings.SampleDistance = d_DefaultSampleDistance;
	}

	
	//AmbientLighting
	if (isStringDouble(AmbientLighting, d_ambientLighting)) {
		volSettings.AmbientLighting = d_ambientLighting; 
	}
	else {
		logParameter("AmbientLighting", AmbientLighting);
		volSettings.AmbientLighting = d_DefaultAmbientLight; 
	}


	
	//DiffuseLighting
	if (isStringDouble(DiffuseLighting, d_diffuseLighting)) {
		volSettings.DiffuseLighting = d_diffuseLighting;
	}
	else {
		logParameter("DiffuseLighting", DiffuseLighting);
		volSettings.DiffuseLighting = d_DefaultDiffuseLight;
	}

	
	//SpecularLighting
	if (isStringDouble(SpecularLighting, d_SpecularLighting)) {
		volSettings.SpecularLighting = d_SpecularLighting;
	}
	else {
		logParameter("SpecularLighting", SpecularLighting);
		volSettings.SpecularLighting = d_DefaultSpecularLighting;
	}


	
	//SpecularPower
	if (isStringDouble(SpecularPower, d_SpecularPower)) {
		volSettings.SpecularLighting = d_SpecularLighting;
	}
	else {
		logParameter("SpecularPower", SpecularPower);
		volSettings.SpecularLighting = d_DefaultSpecularLighting;
	}










}

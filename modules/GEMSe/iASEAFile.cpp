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
#include "iASEAFile.h"

#include "iAConsole.h"
#include "iAFileUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>

const QString iASEAFile::DefaultSMPFileName("sampling.smp");
const QString iASEAFile::DefaultSPSFileName("sampling.sps");
const QString iASEAFile::DefaultCHRFileName("characteristics.chr");
const QString iASEAFile::DefaultCLTFileName("cluster.clt");
const QString iASEAFile::DefaultModalityFileName("modalities.mod");
const QString iASEAFile::DefaultSeedFileName("points.seed");

namespace
{
	const QString FileVersionKey   = "FileVersion";
	const QString FileVersionValue = "1.6";
	
	const QString ModalitiesKey = "Modalities";
	const QString SeedsKey = "Seeds";
	const QString SamplingDataKey = "SamplingData";
	const QString ClusteringDataKey = "ClusteringData";
	const QString LayoutKey = "Layout";
	
	bool AddIfMissing(QSettings const & settings, QString & result, QString const & key)
	{
		if (!settings.contains(key))
		{
			if (!result.isEmpty())
			{
				result.append(", ");
			}
			result.append(key);
			return true;
		}
		return false;
	}
}

iASEAFile::iASEAFile(QString const & fileName):
	m_good(false)
{
	QFile file(fileName);
	if (!file.exists()) {
		DEBUG_LOG(QString("Load Precalculated Data: File '%1' doesn't exist!").arg(fileName));
		return;
	}
	QSettings metaFile(fileName, QSettings::IniFormat );
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Load Precalculated Data: Reading file '%1' failed!").arg(fileName));
		return;
	}
	if (!metaFile.contains(FileVersionKey) || metaFile.value(FileVersionKey).toString() != FileVersionValue)
	{
		DEBUG_LOG(QString("Load Precalculated Data: Precalculated data file: Invalid or missing version descriptor ('%1' expected, '%2' found)!")
			.arg(FileVersionValue)
			.arg((metaFile.contains(FileVersionKey) ? "'"+metaFile.value(FileVersionKey).toString()+"'" : "none")) );
		return;
	}
	QString missingKeys;
	if (AddIfMissing(metaFile, missingKeys, ModalitiesKey) ||
		AddIfMissing(metaFile, missingKeys, SeedsKey) ||
		AddIfMissing(metaFile, missingKeys, SamplingDataKey) ||
		AddIfMissing(metaFile, missingKeys, ClusteringDataKey) ||
		AddIfMissing(metaFile, missingKeys, LayoutKey))
	{
		DEBUG_LOG(QString("Load Precalculated Data: Required setting(s) %1 missing in analysis description file.").arg(missingKeys));
		return;
	}
	m_SEAFileName = fileName;
	QFileInfo fi(fileName);
	m_ModalityFileName   = MakeAbsolute(fi.absolutePath(), metaFile.value(ModalitiesKey).toString());
	m_SeedsFileName      = MakeAbsolute(fi.absolutePath(), metaFile.value(SeedsKey).toString());
	m_SamplingFileName   = MakeAbsolute(fi.absolutePath(), metaFile.value(SamplingDataKey).toString());
	m_ClusteringFileName = MakeAbsolute(fi.absolutePath(), metaFile.value(ClusteringDataKey).toString());
	m_LayoutName         = metaFile.value(LayoutKey).toString();
	m_good = true;
}


iASEAFile::iASEAFile(QString const & modalityFile,
		QString const & seedsFile,
		QString const & smpFile,
		QString const & clusterFile,
		QString const & layout):
	m_ModalityFileName(modalityFile),
	m_SeedsFileName(seedsFile),
	m_SamplingFileName(smpFile),
	m_ClusteringFileName(clusterFile),
	m_LayoutName(layout),
	m_good(true)
{
	
}

void iASEAFile::Store(QString const & fileName)
{
	QSettings metaFile(fileName, QSettings::IniFormat);
	metaFile.setValue(FileVersionKey, FileVersionValue);
	
	m_SEAFileName = fileName;
	QFileInfo fi(fileName);
	QString path(fi.absolutePath());
	metaFile.setValue(ModalitiesKey    , MakeRelative(path, m_ModalityFileName));
	metaFile.setValue(SeedsKey         , MakeRelative(path, m_SeedsFileName));
	metaFile.setValue(SamplingDataKey  , MakeRelative(path, m_SamplingFileName));
	metaFile.setValue(ClusteringDataKey, MakeRelative(path, m_ClusteringFileName));
	metaFile.setValue(LayoutKey        , MakeRelative(path, m_LayoutName));
	
	metaFile.sync();
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Storing precalculated data: File '%1' couldn't be written.").arg(fileName));
	}
}

bool iASEAFile::good() const
{
	return m_good;
}


QString const & iASEAFile::GetModalityFileName() const
{
	return m_ModalityFileName;
}

QString const & iASEAFile::GetSeedsFileName() const
{
	return m_SeedsFileName;
}

QString const & iASEAFile::GetSamplingFileName() const
{
	return m_SamplingFileName;
}

QString const & iASEAFile::GetClusteringFileName() const
{
	return m_ClusteringFileName;
}

QString const & iASEAFile::GetLayoutName() const
{
	return m_LayoutName;
}

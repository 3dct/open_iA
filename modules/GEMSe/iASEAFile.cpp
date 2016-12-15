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
const int DefaultLabelCount = 2;

namespace
{
	const QString FileVersionKey   = "FileVersion";
	const QString FileVersionValue = "1.6.1";
	
	const QString ModalitiesKey = "Modalities";
	const QString LabelCountKey = "LabelCount";
	const QString SamplingDataKey = "SamplingData";
	const QString ClusteringDataKey = "ClusteringData";
	const QString LayoutKey = "Layout";
	const QString ReferenceImageKey = "ReferenceImage";
	const QString HiddenChartsKey = "HiddenCharts";

	void AppendToString(QString & result, QString const & append)
	{
		if (!result.isEmpty())
		{
			result.append(", ");
		}
		result.append(append);
	}
	
	bool AddIfMissing(QSettings const & settings, QString & result, QString const & key)
	{
		if (!settings.contains(key))
		{
			AppendToString(result, key);
			return true;
		}
		return false;
	}
	bool AddIfEmpty(QStringList const & stringlist, QString & result, QString const & key)
	{
		if (stringlist.isEmpty())
		{
			AppendToString(result, key);
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
	QStringList datasetKeys(metaFile.allKeys().filter(SamplingDataKey));
	QString missingKeys;
	if (AddIfMissing(metaFile, missingKeys, ModalitiesKey) ||
		AddIfMissing(metaFile, missingKeys, LabelCountKey) ||
		AddIfMissing(metaFile, missingKeys, ClusteringDataKey) ||
		AddIfMissing(metaFile, missingKeys, LayoutKey) ||
		AddIfEmpty(datasetKeys, missingKeys, SamplingDataKey))
	{
		DEBUG_LOG(QString("Load Precalculated Data: Required setting(s) %1 missing in analysis description file.").arg(missingKeys));
		return;
	}
	m_SEAFileName = fileName;
	QFileInfo fi(fileName);
	m_ModalityFileName   = MakeAbsolute(fi.absolutePath(), metaFile.value(ModalitiesKey).toString());
	bool labelCountOK;
	m_LabelCount		 = metaFile.value(LabelCountKey).toString().toInt(&labelCountOK);
	if (!labelCountOK)
	{
		DEBUG_LOG("Load Precalculated Data: Label Count invalid!");
		return;
	}
	for (QString keyStr : datasetKeys)
	{
		bool ok = false;
		int key = 0;
		if (keyStr != SamplingDataKey) // old precalculated data, only one dataset, without ID!
		{
			key = keyStr.right(keyStr.length() - SamplingDataKey.length()).toInt(&ok);
		}
		if (!ok)
		{
			DEBUG_LOG(QString("Load Precalculated Data: Invalid Dataset identifier: %1 (maybe missing number, ID part: %2?)").arg(keyStr).arg(key));
			return;
		}
		m_Samplings.insert(key, MakeAbsolute(fi.absolutePath(), metaFile.value(keyStr).toString()));
	}
	m_ClusteringFileName = MakeAbsolute(fi.absolutePath(), metaFile.value(ClusteringDataKey).toString());
	m_LayoutName         = metaFile.value(LayoutKey).toString();
	if (metaFile.contains(ReferenceImageKey))
	{
		m_ReferenceImage = MakeAbsolute(fi.absolutePath(), metaFile.value(ReferenceImageKey).toString());
	}
	if (metaFile.contains(HiddenChartsKey))
	{
		m_HiddenCharts = metaFile.value(HiddenChartsKey).toString();
	}
	m_good = true;
}


iASEAFile::iASEAFile(
		QString const & modalityFile,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & clusterFile,
		QString const & layout,
		QString const & referenceImage,
		QString const & hiddenCharts):
	m_ModalityFileName(modalityFile),
	m_LabelCount(labelCount),
	m_Samplings(samplings),
	m_ClusteringFileName(clusterFile),
	m_LayoutName(layout),
	m_ReferenceImage(referenceImage),
	m_HiddenCharts(hiddenCharts),
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
	metaFile.setValue(LabelCountKey    , m_LabelCount);
	for (int key : m_Samplings.keys())
	{
		metaFile.setValue(SamplingDataKey + QString::number(key), MakeRelative(path, m_Samplings[key]));
	}
	metaFile.setValue(ClusteringDataKey, MakeRelative(path, m_ClusteringFileName));
	metaFile.setValue(LayoutKey        , m_LayoutName);
	if (!m_ReferenceImage.isEmpty())
	{
		metaFile.setValue(ReferenceImageKey, MakeRelative(path, m_ReferenceImage));
	}
	if (!m_HiddenCharts.isEmpty())
	{
		metaFile.setValue(HiddenChartsKey, m_HiddenCharts);
	}
	
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

int iASEAFile::GetLabelCount() const
{
	return m_LabelCount;
}

QMap<int, QString> const & iASEAFile::GetSamplings() const
{
	return m_Samplings;
}

QString const & iASEAFile::GetClusteringFileName() const
{
	return m_ClusteringFileName;
}

QString const & iASEAFile::GetLayoutName() const
{
	return m_LayoutName;
}

QString const & iASEAFile::GetReferenceImage() const
{
	return m_ReferenceImage;
}

QString const & iASEAFile::GetHiddenCharts() const
{
	return m_HiddenCharts;
}

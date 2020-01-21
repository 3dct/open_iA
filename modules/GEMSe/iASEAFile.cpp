/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iASEAFile.h"

#include <iAConsole.h>
#include <io/iAFileUtils.h>

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
	const QString ColorThemeKey = "ColorTheme";
	const QString LabelNamesKey = "LabelNames";

	void AppendToString(QString & result, QString const & append)
	{
		if (!result.isEmpty())
		{
			result.append(", ");
		}
		result.append(append);
	}

	void AddIfMissing(QSettings const & settings, QString & result, QString const & key)
	{
		if (!settings.contains(key))
		{
			AppendToString(result, key);
		}
	}
	void AddIfEmpty(QStringList const & stringlist, QString & result, QString const & key)
	{
		if (stringlist.isEmpty())
		{
			AppendToString(result, key);
		}
	}
}

iASEAFile::iASEAFile(QString const & fileName):
	m_good(false)
{
	QFile file(fileName);
	if (!file.exists()) {
		DEBUG_LOG(QString("Load precalculated GEMSe data: File '%1' doesn't exist!").arg(fileName));
		return;
	}
	QSettings metaFile(fileName, QSettings::IniFormat );
	metaFile.setIniCodec("UTF-8");
	load(metaFile, fileName, true);
}

iASEAFile::iASEAFile(
		QString const & modalityFile,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & clusterFile,
		QString const & layout,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorTheme,
		QString const & labelNames):
	m_modalityFileName(modalityFile),
	m_labelCount(labelCount),
	m_samplings(samplings),
	m_clusteringFileName(clusterFile),
	m_layoutName(layout),
	m_refImg(refImg),
	m_hiddenCharts(hiddenCharts),
	m_colorTheme(colorTheme),
	m_labelNames(labelNames),
	m_good(true)
{
}

iASEAFile::iASEAFile(QSettings const & metaFile, QString const & fileName)
{
	load(metaFile, fileName, false);
}

void iASEAFile::load(QSettings const & metaFile, QString const & fileName, bool modalityRequired)
{
	m_fileName = fileName;
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Loading GEMSe data from file '%1' failed!").arg(fileName));
		return;
	}
	if (!metaFile.contains(FileVersionKey) || metaFile.value(FileVersionKey).toString() != FileVersionValue)
	{
		DEBUG_LOG(QString("Loading GEMSe data from file (%1) failed: Invalid or missing version descriptor ('%2' expected, '%3' found)!")
			.arg(fileName)
			.arg(FileVersionValue)
			.arg((metaFile.contains(FileVersionKey) ? "'" + metaFile.value(FileVersionKey).toString() + "'" : "none")));
		return;
	}
	QStringList datasetKeys(metaFile.allKeys().filter(SamplingDataKey));
	QString missingKeys;
	if (modalityRequired)
		AddIfMissing(metaFile, missingKeys, ModalitiesKey);
	AddIfMissing(metaFile, missingKeys, LabelCountKey);
	AddIfMissing(metaFile, missingKeys, ClusteringDataKey);
	AddIfMissing(metaFile, missingKeys, LayoutKey);
	AddIfEmpty(datasetKeys, missingKeys, SamplingDataKey);
	if (missingKeys.size() > 0)
	{
		DEBUG_LOG(QString("Loading GEMSe data from file (%1) failed: Required setting(s) %2 missing or empty!").arg(fileName).arg(missingKeys));
		return;
	}
	QFileInfo fi(fileName);
	if (modalityRequired)
		m_modalityFileName = MakeAbsolute(fi.absolutePath(), metaFile.value(ModalitiesKey).toString());

	bool labelCountOK;
	m_labelCount = metaFile.value(LabelCountKey).toString().toInt(&labelCountOK);
	if (!labelCountOK)
	{
		DEBUG_LOG(QString("Loading GEMSe data from file (%1) failed: Value '%2' is not a valid label count!")
			.arg(fileName).arg(metaFile.value(LabelCountKey).toString()));
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
			DEBUG_LOG(QString("Loading GEMSe data from file (%1) failed: Invalid Dataset identifier: %2 (maybe missing number, ID part: %3?).")
				.arg(fileName).arg(keyStr).arg(key));
			return;
		}
		m_samplings.insert(key, MakeAbsolute(fi.absolutePath(), metaFile.value(keyStr).toString()));
	}
	QList<int> keys = m_samplings.keys();
	std::sort(keys.begin(), keys.end());
	if (keys[0] != 0 || keys[keys.size() - 1] != keys.size() - 1)
	{
		DEBUG_LOG(QString("Loading GEMSe data from file (%1) failed: Incoherent sampling indices, or not starting at 0: [%1..%2].")
			.arg(fileName).arg(keys[0]).arg(keys[keys.size() - 1]));
		return;
	}
	m_clusteringFileName = MakeAbsolute(fi.absolutePath(), metaFile.value(ClusteringDataKey).toString());
	m_layoutName = metaFile.value(LayoutKey).toString();
	if (metaFile.contains(ReferenceImageKey))
	{
		m_refImg = MakeAbsolute(fi.absolutePath(), metaFile.value(ReferenceImageKey).toString());
	}
	if (metaFile.contains(HiddenChartsKey))
	{
		m_hiddenCharts = metaFile.value(HiddenChartsKey).toString();
	}
	if (metaFile.contains(ColorThemeKey))
	{
		m_colorTheme = metaFile.value(ColorThemeKey).toString();
	}
	if (metaFile.contains(LabelNamesKey))
	{
		m_labelNames = metaFile.value(LabelNamesKey).toString();
	}
	m_good = true;
}

void iASEAFile::save(QString const & fileName)
{
	QSettings metaFile(fileName, QSettings::IniFormat);
	metaFile.setIniCodec("UTF-8");
	metaFile.setValue(ModalitiesKey, MakeRelative(QFileInfo(fileName).absolutePath(), m_modalityFileName));
	save(metaFile, fileName);
}

void iASEAFile::save(QSettings & metaFile, QString const & fileName)
{
	m_fileName = fileName;
	QFileInfo fi(fileName);
	QString path(fi.absolutePath());
	metaFile.setValue(FileVersionKey, FileVersionValue);
	metaFile.setValue(LabelCountKey, m_labelCount);
	for (int key : m_samplings.keys())
	{
		metaFile.setValue(SamplingDataKey + QString::number(key), MakeRelative(path, m_samplings[key]));
	}
	metaFile.setValue(ClusteringDataKey, MakeRelative(path, m_clusteringFileName));
	metaFile.setValue(LayoutKey, m_layoutName);
	if (!m_refImg.isEmpty())
	{
		metaFile.setValue(ReferenceImageKey, MakeRelative(path, m_refImg));
	}
	if (!m_hiddenCharts.isEmpty())
	{
		metaFile.setValue(HiddenChartsKey, m_hiddenCharts);
	}
	metaFile.setValue(ColorThemeKey, m_colorTheme);
	metaFile.setValue(LabelNamesKey, m_labelNames);

	metaFile.sync();
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Storing GEMSe data: File '%1' couldn't be written.").arg(fileName));
	}
}

bool iASEAFile::good() const
{
	return m_good;
}

QString const & iASEAFile::modalityFileName() const
{
	return m_modalityFileName;
}

int iASEAFile::labelCount() const
{
	return m_labelCount;
}

QMap<int, QString> const & iASEAFile::samplings() const
{
	return m_samplings;
}

QString const & iASEAFile::clusteringFileName() const
{
	return m_clusteringFileName;
}

QString const & iASEAFile::layoutName() const
{
	return m_layoutName;
}

QString const & iASEAFile::referenceImage() const
{
	return m_refImg;
}

QString const & iASEAFile::hiddenCharts() const
{
	return m_hiddenCharts;
}

QString const & iASEAFile::labelNames() const
{
	return m_labelNames;
}

QString const & iASEAFile::colorTheme() const
{
	return m_colorTheme;
}

QString const & iASEAFile::fileName() const
{
	return m_fileName;
}

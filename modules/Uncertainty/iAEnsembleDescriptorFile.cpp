/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAEnsembleDescriptorFile.h"

#include <iAConsole.h>
#include <iAStringHelper.h>
#include <io/iAFileUtils.h>

#include <QFile>
#include <QFileInfo>
#include <QSettings>

const QString iAEnsembleDescriptorFile::DefaultSMPFileName("sampling.smp");
const QString iAEnsembleDescriptorFile::DefaultSPSFileName("sampling.sps");
const QString iAEnsembleDescriptorFile::DefaultCHRFileName("characteristics.chr");
const QString iAEnsembleDescriptorFile::DefaultModalityFileName("modalities.mod");
const int DefaultLabelCount = 2;

namespace
{
	const QString FileVersionKey   = "FileVersion";
	const QString FileVersionValue = "1.6.1";
	
	const QString ModalitiesKey = "Modalities";
	const QString LabelCountKey = "LabelCount";
	const QString SamplingDataKey = "SamplingData";
	const QString LayoutKey = "Layout";
	const QString ReferenceImageKey = "ReferenceImage";
	const QString HiddenChartsKey = "HiddenCharts";
	const QString ColorThemeKey = "ColorTheme";
	const QString LabelNamesKey = "LabelNames";
	const QString SubEnsembleKey = "SubEnsemble";

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

iAEnsembleDescriptorFile::iAEnsembleDescriptorFile(QString const & fileName):
	m_good(false)
{
	QFile file(fileName);
	if (!file.exists())
	{
		DEBUG_LOG(QString("Ensemble loading: File '%1' doesn't exist!").arg(fileName));
		return;
	}
	QSettings metaFile(fileName, QSettings::IniFormat );
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Ensemble loading: Reading file '%1' failed!").arg(fileName));
		return;
	}
	if (!metaFile.contains(FileVersionKey) || metaFile.value(FileVersionKey).toString() != FileVersionValue)
	{
		DEBUG_LOG(QString("Ensemble loading: Ensemble file: Invalid or missing version descriptor ('%1' expected, '%2' found)!")
			.arg(FileVersionValue)
			.arg((metaFile.contains(FileVersionKey) ? "'"+metaFile.value(FileVersionKey).toString()+"'" : "none")) );
		return;
	}
	QStringList datasetKeys(metaFile.allKeys().filter(SamplingDataKey));
	QString missingKeys;
	if (AddIfMissing(metaFile, missingKeys, ModalitiesKey) ||
		AddIfMissing(metaFile, missingKeys, LabelCountKey) ||
		AddIfMissing(metaFile, missingKeys, LayoutKey) ||
		AddIfEmpty(datasetKeys, missingKeys, SamplingDataKey))
	{
		DEBUG_LOG(QString("Ensemble loading: Required setting(s) %1 missing in ensemble description file.").arg(missingKeys));
		return;
	}
	m_fileName = fileName;
	QFileInfo fi(fileName);
	m_ModalityFileName   = MakeAbsolute(fi.absolutePath(), metaFile.value(ModalitiesKey).toString());
	bool labelCountOK;
	m_LabelCount		 = metaFile.value(LabelCountKey).toString().toInt(&labelCountOK);
	if (!labelCountOK)
	{
		DEBUG_LOG("Ensemble loading: Label Count invalid!");
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
			DEBUG_LOG(QString("Ensemble loading: Invalid Dataset identifier: %1 (maybe missing number, ID part: %2?)").arg(keyStr).arg(key));
			return;
		}
		m_Samplings.insert(key, MakeAbsolute(fi.absolutePath(), metaFile.value(keyStr).toString()));
	}
	QList<int> keys = m_Samplings.keys();
	std::sort(keys.begin(), keys.end());
	if (keys[0] != 0 || keys[keys.size() - 1] != keys.size() - 1)
	{
		DEBUG_LOG(QString("Ensemble loading: Incoherent sampling indices, or not starting at 0: [%1..%2]").arg(keys[0]).arg(keys[keys.size() - 1]));
		return;
	}
	m_LayoutName         = metaFile.value(LayoutKey).toString();
	if (metaFile.contains(ReferenceImageKey))
	{
		m_RefImg = MakeAbsolute(fi.absolutePath(), metaFile.value(ReferenceImageKey).toString());
	}
	if (metaFile.contains(HiddenChartsKey))
	{
		m_HiddenCharts = metaFile.value(HiddenChartsKey).toString();
	}
	if (metaFile.contains(ColorThemeKey))
	{
		m_ColorTheme = metaFile.value(ColorThemeKey).toString();
	}
	if (metaFile.contains(LabelNamesKey))
	{
		m_LabelNames = metaFile.value(LabelNamesKey).toString();
	}

	QStringList subEnsembleKeys(metaFile.allKeys().filter(SubEnsembleKey));
	for (QString keyStr : subEnsembleKeys)
	{
		bool ok = false;
		int key = 0;
		key = keyStr.right(keyStr.length() - SubEnsembleKey.length()).toInt(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Ensemble loading: Invalid Subset identifier: %1 (maybe missing number, ID part: %2?)").arg(keyStr).arg(key));
			return;
		}
		QStringList idStrings = metaFile.value(keyStr).toString().split(",");
		QVector<int> memberIDs;
		for (QString idString : idStrings)
		{
			int val = idString.toInt(&ok);
			if (!ok)
			{
				DEBUG_LOG(QString("Ensemble loading: Invalid Subset member ID: %1 (number part: %2)").arg(idString).arg(val));
				return;
			}
			memberIDs.push_back(val);
		}
		AddSubEnsemble(key, memberIDs);
	}
	
	m_good = true;
}


iAEnsembleDescriptorFile::iAEnsembleDescriptorFile(
		QString const & modalityFile,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & layout,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorTheme,
		QString const & labelNames):
	m_ModalityFileName(modalityFile),
	m_LabelCount(labelCount),
	m_Samplings(samplings),
	m_LayoutName(layout),
	m_RefImg(refImg),
	m_HiddenCharts(hiddenCharts),
	m_ColorTheme(colorTheme),
	m_LabelNames(labelNames),
	m_good(true)
{
}

void iAEnsembleDescriptorFile::Store(QString const & fileName)
{
	QSettings metaFile(fileName, QSettings::IniFormat);
	metaFile.setValue(FileVersionKey, FileVersionValue);
	
	m_fileName = fileName;
	QFileInfo fi(fileName);
	QString path(fi.absolutePath());
	metaFile.setValue(ModalitiesKey    , MakeRelative(path, m_ModalityFileName));
	metaFile.setValue(LabelCountKey    , m_LabelCount);
	for (int key : m_Samplings.keys())
	{
		metaFile.setValue(SamplingDataKey + QString::number(key), MakeRelative(path, m_Samplings[key]));
	}
	metaFile.setValue(LayoutKey        , m_LayoutName);
	if (!m_RefImg.isEmpty())
	{
		metaFile.setValue(ReferenceImageKey, MakeRelative(path, m_RefImg));
	}
	if (!m_HiddenCharts.isEmpty())
	{
		metaFile.setValue(HiddenChartsKey, m_HiddenCharts);
	}
	if (!m_ColorTheme.isEmpty())
	{
		metaFile.setValue(ColorThemeKey, m_ColorTheme);
	}
	if (!m_LabelNames.isEmpty())
	{
		metaFile.setValue(LabelNamesKey, m_LabelNames);
	}

	for (int i = 0; i < m_subEnsembles.size(); ++i)
	{
		metaFile.setValue(SubEnsembleKey + QString::number(m_subEnsembleID[i]), join(m_subEnsembles[i], ","));
	}
	
	metaFile.sync();
	if (metaFile.status() != QSettings::NoError)
	{
		DEBUG_LOG(QString("Ensemble storing: File '%1' couldn't be written.").arg(fileName));
	}
}

bool iAEnsembleDescriptorFile::good() const
{
	return m_good;
}

QString const & iAEnsembleDescriptorFile::FileName() const
{
	return m_fileName;
}


QString const & iAEnsembleDescriptorFile::ModalityFileName() const
{
	return m_ModalityFileName;
}

int iAEnsembleDescriptorFile::LabelCount() const
{
	return m_LabelCount;
}

QMap<int, QString> const & iAEnsembleDescriptorFile::Samplings() const
{
	return m_Samplings;
}

QString const & iAEnsembleDescriptorFile::LayoutName() const
{
	return m_LayoutName;
}

QString const & iAEnsembleDescriptorFile::ReferenceImage() const
{
	return m_RefImg;
}

QString const & iAEnsembleDescriptorFile::HiddenCharts() const
{
	return m_HiddenCharts;
}

QString const & iAEnsembleDescriptorFile::LabelNames() const
{
	return m_LabelNames;
}

QString const & iAEnsembleDescriptorFile::ColorTheme() const
{
	return m_ColorTheme;
}


size_t iAEnsembleDescriptorFile::SubEnsembleCount() const
{
	return m_subEnsembles.size();
}

QVector<int> iAEnsembleDescriptorFile::SubEnsemble(size_t idx) const
{
	return m_subEnsembles[idx];
}

int iAEnsembleDescriptorFile::SubEnsembleID(size_t idx) const
{
	return m_subEnsembleID[idx];
}

void iAEnsembleDescriptorFile::AddSubEnsemble(int id, QVector<int> const & members)
{
	m_subEnsembleID.push_back(id);
	m_subEnsembles.push_back(members);
}
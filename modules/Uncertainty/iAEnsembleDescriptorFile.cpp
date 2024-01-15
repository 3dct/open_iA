// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAEnsembleDescriptorFile.h"

#include <iALog.h>
#include <iAStringHelper.h>
#include <iAFileUtils.h>

#include <QFile>
#include <QFileInfo>
#include <QSettings>

const QString iAEnsembleDescriptorFile::DefaultSMPFileName("sampling.smp");
const QString iAEnsembleDescriptorFile::DefaultSPSFileName("sampling.sps");
const QString iAEnsembleDescriptorFile::DefaultCHRFileName("characteristics.chr");

namespace
{
	const QString LabelCountKey = "LabelCount";
	const QString SamplingDataKey = "SamplingData";
	const QString LayoutKey = "Layout";
	const QString ReferenceImageKey = "ReferenceImage";
	const QString HiddenChartsKey = "HiddenCharts";
	const QString ColorThemeKey = "ColorTheme";
	const QString LabelNamesKey = "LabelNames";
	const QString SubEnsembleKey = "SubEnsemble";

	void appendToString(QString & result, QString const & append)
	{
		if (!result.isEmpty())
		{
			result.append(", ");
		}
		result.append(append);
	}

	bool addIfMissing(QSettings const & settings, QString & result, QString const & key)
	{
		if (!settings.contains(key))
		{
			appendToString(result, key);
			return true;
		}
		return false;
	}
	bool addIfEmpty(QStringList const & stringlist, QString & result, QString const & key)
	{
		if (stringlist.isEmpty())
		{
			appendToString(result, key);
			return true;
		}
		return false;
	}
}

iAEnsembleDescriptorFile::iAEnsembleDescriptorFile(QSettings const & metaFile, QString const& fileName) :
	m_good(false)
{
	QStringList datasetKeys(metaFile.allKeys().filter(SamplingDataKey));
	QString missingKeys;
	if (addIfMissing(metaFile, missingKeys, LabelCountKey) ||
		addIfMissing(metaFile, missingKeys, LayoutKey) ||
		addIfEmpty(datasetKeys, missingKeys, SamplingDataKey))
	{
		LOG(lvlError, QString("Ensemble loading: Required setting(s) %1 missing in ensemble description file.").arg(missingKeys));
		return;
	}
	m_fileName = fileName;
	QFileInfo fi(fileName);
	bool labelCountOK;
	m_LabelCount		 = metaFile.value(LabelCountKey).toString().toInt(&labelCountOK);
	if (!labelCountOK)
	{
		LOG(lvlError, "Ensemble loading: Label Count invalid!");
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
			LOG(lvlError, QString("Ensemble loading: Invalid Dataset identifier: %1 (maybe missing number, ID part: %2?)").arg(keyStr).arg(key));
			return;
		}
		m_Samplings.insert(key, MakeAbsolute(fi.absolutePath(), metaFile.value(keyStr).toString()));
	}
	QList<int> keys = m_Samplings.keys();
	std::sort(keys.begin(), keys.end());
	if (keys[0] != 0 || keys[keys.size() - 1] != keys.size() - 1)
	{
		LOG(lvlError, QString("Ensemble loading: Incoherent sampling indices, or not starting at 0: [%1..%2]").arg(keys[0]).arg(keys[keys.size() - 1]));
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
			LOG(lvlError, QString("Ensemble loading: Invalid Subset identifier: %1 (maybe missing number, ID part: %2?)").arg(keyStr).arg(key));
			return;
		}
		QStringList idStrings = metaFile.value(keyStr).toString().split(",");
		QVector<int> memberIDs;
		for (QString idString : idStrings)
		{
			int val = idString.toInt(&ok);
			if (!ok)
			{
				LOG(lvlError, QString("Ensemble loading: Invalid Subset member ID: %1 (number part: %2)").arg(idString).arg(val));
				return;
			}
			memberIDs.push_back(val);
		}
		addSubEnsemble(key, memberIDs);
	}

	m_good = true;
}

/*
iAEnsembleDescriptorFile::iAEnsembleDescriptorFile(
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & layout,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorTheme,
		QString const & labelNames):
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
*/

void iAEnsembleDescriptorFile::store(QSettings & metaFile, QString const & fileName)
{
	m_fileName = fileName;
	QFileInfo fi(fileName);
	QString path(fi.absolutePath());
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
		metaFile.setValue(SubEnsembleKey + QString::number(m_subEnsembleID[i]),
			joinNumbersAsString(m_subEnsembles[i], ","));
	}
}

bool iAEnsembleDescriptorFile::good() const
{
	return m_good;
}

QString const & iAEnsembleDescriptorFile::fileName() const
{
	return m_fileName;
}

int iAEnsembleDescriptorFile::labelCount() const
{
	return m_LabelCount;
}

QMap<int, QString> const & iAEnsembleDescriptorFile::samplings() const
{
	return m_Samplings;
}

QString const & iAEnsembleDescriptorFile::layoutName() const
{
	return m_LayoutName;
}

QString const & iAEnsembleDescriptorFile::referenceImage() const
{
	return m_RefImg;
}

QString const & iAEnsembleDescriptorFile::hiddenCharts() const
{
	return m_HiddenCharts;
}

QString const & iAEnsembleDescriptorFile::labelNames() const
{
	return m_LabelNames;
}

QString const & iAEnsembleDescriptorFile::colorTheme() const
{
	return m_ColorTheme;
}

int iAEnsembleDescriptorFile::subEnsembleCount() const
{
	return m_subEnsembles.size();
}

QVector<int> iAEnsembleDescriptorFile::subEnsemble(int idx) const
{
	return m_subEnsembles[idx];
}

int iAEnsembleDescriptorFile::subEnsembleID(int idx) const
{
	return m_subEnsembleID[idx];
}

void iAEnsembleDescriptorFile::addSubEnsemble(int id, QVector<int> const & members)
{
	m_subEnsembleID.push_back(id);
	m_subEnsembles.push_back(members);
}

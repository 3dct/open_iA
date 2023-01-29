// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QMap>

class QSettings;

class iASEAFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;
	static const QString DefaultCLTFileName;
	static const QString DefaultModalityFileName;

	iASEAFile(QString const & seaFileName);
	iASEAFile(
		QString const & modFileName,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & cltFileName,
		QString const & layoutName,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorThemeName,
		QString const & labelNames
	);
	//! Takes given settings and reads GEMSe configuration from it.
	//! Assumes that modalities are already loaded / not specified via single "Modalities" file name entry
	iASEAFile(QSettings const & metaFile, QString const & fileName);
	void save(QString const & seaFileName);
	//! Store everything in given settings.
	void save(QSettings & metaFile, QString const & fileName);
	void load(QSettings const & metaFile, QString const & fileName, bool modalityRequired);
	QString const & modalityFileName() const;
	int labelCount() const;
	QMap<int, QString> const & samplings() const;
	QString const & clusteringFileName() const;
	QString const & layoutName() const;
	QString const & referenceImage() const;
	QString const & hiddenCharts() const;
	QString const & labelNames() const;
	QString const & colorTheme() const;

	bool good() const;
	QString const & fileName() const;
private:
	iASEAFile(iASEAFile const & other) = delete;
	iASEAFile& operator=(iASEAFile const & other) = delete;
	QString m_fileName;
	QString m_modalityFileName;
	int m_labelCount;
	QMap<int, QString> m_samplings;
	QString m_clusteringFileName;
	QString m_layoutName;
	QString m_refImg;
	QString m_hiddenCharts;
	QString m_colorTheme;
	QString m_labelNames;

	bool m_good;
};

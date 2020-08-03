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

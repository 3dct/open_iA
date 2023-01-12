/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <QMap>
#include <QString>
#include <QVector>

class QSettings;

class iAEnsembleDescriptorFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;

	iAEnsembleDescriptorFile(QSettings const& metaFile, QString const & ensembleFileName);
	/*
	iAEnsembleDescriptorFile(
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & layoutName,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorThemeName,
		QString const & labelNames
	);
	*/
	void store(QSettings& metaFile, QString const & ensembleFileName);
	QString const & fileName() const;
	int labelCount() const;
	QMap<int, QString> const & samplings() const;
	QString const & layoutName() const;
	QString const & referenceImage() const;
	QString const & hiddenCharts() const;
	QString const & labelNames() const;
	QString const & colorTheme() const;

	int subEnsembleCount() const;
	QVector<int> subEnsemble(int idx) const;
	int subEnsembleID(int idx) const;

	void addSubEnsemble(int id, QVector<int> const & members);

	bool good() const;
private:
	int m_LabelCount;
	QMap<int, QString> m_Samplings;
	QString m_LayoutName;
	QString m_fileName;
	QString m_RefImg;
	QString m_HiddenCharts;
	QString m_ColorTheme;
	QString m_LabelNames;

	QVector<QVector<int> > m_subEnsembles;
	QVector<int> m_subEnsembleID;

	bool m_good;
};

/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

#include <QMap>
#include <QString>
#include <QVector>

class iAEnsembleDescriptorFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;
	static const QString DefaultModalityFileName;
	static const int DefaultLabelCount;

	iAEnsembleDescriptorFile(QString const & ensembleFileName);
	iAEnsembleDescriptorFile(
		QString const & modFileName,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & layoutName,
		QString const & refImg,
		QString const & hiddenCharts,
		QString const & colorThemeName,
		QString const & labelNames
	);
	void Store(QString const & ensembleFileName);
	QString const & FileName() const;
	QString const & ModalityFileName() const;
	int LabelCount() const;
	QMap<int, QString> const & Samplings() const;
	QString const & LayoutName() const;
	QString const & ReferenceImage() const;
	QString const & HiddenCharts() const;
	QString const & LabelNames() const;
	QString const & ColorTheme() const;

	size_t SubEnsembleCount() const;
	QVector<int> SubEnsemble(size_t idx) const;
	int SubEnsembleID(size_t idx) const;

	void AddSubEnsemble(int id, QVector<int> const & members);

	bool good() const;
private:
	QString m_ModalityFileName;
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

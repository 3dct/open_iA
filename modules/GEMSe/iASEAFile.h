/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <QString>
#include <QMap>

class iASEAFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;
	static const QString DefaultCLTFileName;
	static const QString DefaultModalityFileName;
	static const int DefaultLabelCount;

	iASEAFile(QString const & seaFileName);
	iASEAFile(
		QString const & modFileName,
		int labelCount,
		QMap<int, QString> const & samplings,
		QString const & cltFileName,
		QString const & layoutName,
		QString const & referenceImage,
		QString const & hiddenCharts
	);
	void Store(QString const & seaFileName);
	QString const & GetModalityFileName() const;
	int GetLabelCount() const;
	QMap<int, QString> const & GetSamplings() const;
	QString const & GetClusteringFileName() const;
	QString const & GetLayoutName() const;
	QString const & GetReferenceImage() const;
	QString const & GetHiddenCharts() const;

	bool good() const;
	// QString const & GetSEAFileName();
private:
	QString m_ModalityFileName;
	int m_LabelCount;
	QMap<int, QString> m_Samplings;
	QString m_ClusteringFileName;
	QString m_LayoutName;
	QString m_SEAFileName;
	QString m_ReferenceImage;
	QString m_HiddenCharts;

	bool m_good;
};

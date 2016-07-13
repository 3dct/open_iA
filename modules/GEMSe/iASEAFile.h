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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include <QString>

class iASEAFile
{
public:

	static const QString DefaultSMPFileName;
	static const QString DefaultSPSFileName;
	static const QString DefaultCHRFileName;
	static const QString DefaultCLTFileName;
	static const QString DefaultModalityFileName;
	static const QString DefaultSeedFileName;

	iASEAFile(QString const & seaFileName);
	iASEAFile(
		QString const & modFileName,
		QString const & seedsFileName,
		QString const & smpFileName,
		QString const & cltFileName,
		QString const & layoutName
	);
	void Store(QString const & seaFileName);
	QString const & GetModalityFileName() const;
	QString const & GetSeedsFileName() const;
	QString const & GetSamplingFileName() const;
	QString const & GetClusteringFileName() const;
	QString const & GetLayoutName() const;

	bool good() const;
	// QString const & GetSEAFileName();
private:
	QString m_ModalityFileName;
	QString m_SeedsFileName;
	QString m_SamplingFileName;
	QString m_ClusteringFileName;
	QString m_LayoutName;
	QString m_SEAFileName;

	bool m_good;
};

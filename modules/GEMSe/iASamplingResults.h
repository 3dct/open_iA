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
#pragma once

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAAttributes;
class iASingleResult;

class iASamplingResults
{
public:
	iASamplingResults(QSharedPointer<iAAttributes> attr, QString const & samplingMethod);
	static QSharedPointer<iASamplingResults> Load(QString const & metaFileName);
	bool Store(QString const & smpFileName,
		QString const & parameterSetFileName,
		QString const & characteristicsFileName);
	int size() const;
	QSharedPointer<iASingleResult> Get(int i) const;

	void AddResult(QSharedPointer<iASingleResult> result);

	QVector<QSharedPointer<iASingleResult> > const & GetResults() const;

	QSharedPointer<iAAttributes> GetAttributes();

	QString GetFileName() const;
private:
	QSharedPointer<iAAttributes> m_attributes;
	QVector<QSharedPointer<iASingleResult> > m_results;
	QString m_samplingMethod;
	QString m_fileName;       // smp fileName

	bool LoadInternal(QString const & parameterSetFileName, QString const & characteristicsFileName);
	bool StoreAttributes(int type, QString const & fileName, bool id);
};

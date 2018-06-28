/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QString>
#include <QStringList>
#include <QTableWidget>

class DataTable : public QTableWidget
{
public:
	DataTable();
	//! reads table entries from csv file into qtable widget
	bool readTableEntries(const QString &fName, const uint rowCount, const QString & colSeparator,
		const uint skipLinesStart, const bool readHeaders, bool addAutoID, QString const & encoding);
	//! clears the table content and sets it to zero rows
	void clearTable();
	void setHeader(const QStringList &headerEntries);
	QString getLastEncoding() const;
	const QStringList & getHeaders() const;

	QString AutoIDColumnName = "AutoID";
private:
	QStringList m_headerEntries; //!< list of column header names
	QString m_LastEncoding; //!< encoding used when last reading the csv file
	size_t m_autoRID;       //!< counter variable for automatically assigned row ID

	//! disable copy constructor
	DataTable(const DataTable &other) =delete;

	void readTableValues(size_t const rowCount, QTextStream &file, bool addAutoID, const QString & colSeparator);
	//! adding file entry to table (+ optional auto id entry)
	void addLineToTable(QStringList const &tableEntries, size_t row, bool addAutoID);
	void prepareHeader(uint skipLinesStart, QTextStream &file, bool readHeaders, bool addAutoID, const QString & colSeparator);
};

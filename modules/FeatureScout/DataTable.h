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

#include "csv_config.h"

#include <QString>
#include <QStringList>
#include <QSharedPointer>
#include <QTableWidget>

class QFile;

namespace DataIO
{

	class DataTable : public QTableWidget
	{
	public:
		DataTable();
		void initToDefault();
		~DataTable();
		void initTable();

		inline const DataTable &getPreviewTable() const {
			return *this;
		}

		void addLineToTable(const QSharedPointer<QStringList> &tableEntries);

		//reading rows from a file;
		bool readTableEntries(const QString &fName, const uint rowCount, uint colCount, const int headerNr,
			const uint StartLine, const bool readHeaders, bool insertID, QString const & encoding);

		void readTableValues(const uint &rowCount, QTextStream &file, QString &el_line);
		void prepareHeader(uint headerLine, QString &el_line, QTextStream &file, const bool &readHeaders, bool insertID);
		bool prepareFile(const QString & fName, QFile &file);
		void prepareTable(const int rowCount, const int colCount, const int headerLineNr);
		void clearTable();
		void resetIndizes();
		void setHeader(const QStringList &headerEntries);
		void setColSeparator(const csvConfig::csvSeparator & separator);
		QString getLastEncoding() const;

		inline const QStringList &getHeaders() {
			return *this->m_headerEntries;
		}

	protected:
		QSharedPointer<QStringList> m_currentEntry;
		QSharedPointer<QStringList> m_headerEntries;

		QTableWidgetItem *m_currentItem;
		QItemSelectionModel *m_variableModel;
		QModelIndex m_item;
		uint m_rowInd;
		uint m_colInd;
		uint m_currHeaderLineNr;
		bool isInitialized;
		bool isDataFilled;

		int m_colCount;

		//insert auto row ID
		bool insertROW_ID;
		QString m_FileSeperator;
		QString m_FileName;
		QString m_LastEncoding;

		//name of first column
		QString m_rowID;

		//row ID automatically assigned
		uint m_autoRID;
	private:
		//disable copy constructor
		DataTable(const DataTable &other);

	};

}

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
#include "DataTable.h"

#include <QFile>
#include <QMessageBox>
#include <QTextCodec>
#include <QTextStream>

namespace DataIO {

	DataTable::DataTable()
	{
		initToDefault();
	}

	void DataTable::initToDefault()
	{
		this->m_currentItem = 0;
		this->m_variableModel = 0;
		this->m_currHeaderLineNr = 0;
		this->m_colInd = 0;
		this->m_rowInd = 0;
		this->m_FileSeperator = ",";
		this->isDataFilled = false;
		this->m_autoRID = 0;
		this->m_rowID = "AUTO_ID";
		this->insertROW_ID = false;

		//minimum to values to show
		this->m_colCount = 31;
	}


	DataTable::~DataTable()
	{
		if (!this->m_currentItem) {
			delete this->m_currentItem;
			this->m_currentItem = 0;
		}

		this->m_variableModel = 0;
	}

	void DataTable::initTable()
	{
		if (!this->m_currentItem) {
			this->m_currentItem = new QTableWidgetItem();
		}

		if (!this->m_currentEntry) {
			this->m_currentEntry = QSharedPointer<QStringList>(new QStringList());
		}

		if (!this->m_variableModel) {
			this->m_variableModel = this->selectionModel();
		}

		if (!this->m_headerEntries) {
			this->m_headerEntries = QSharedPointer<QStringList>(new QStringList());
		}

	}

	void DataTable::prepareTable(const int rowCount, const int colCount, const int headerLineNr)
	{
		//this->initToDefault();
		this->initTable();
		this->setRowCount((int)rowCount);
		this->setColumnCount((int)colCount);
		this->setSortingEnabled(false);
		this->setShowGrid(true);
		this->setEnabled(false);
		this->m_currHeaderLineNr = headerLineNr;
	}


	//to clear the table when adding new entries
	void DataTable::clearTable()
	{
		if (isDataFilled) {
			this->clear();
			this->model()->removeRows(0, this->rowCount());
			resetIndizes();
			isDataFilled = false;
		}

	}

	void DataTable::resetIndizes()
	{
		this->m_rowInd = 0;
		this->m_colInd = 0;
	}

	//adding file entry to table + first column is auto id;
	void DataTable::addLineToTable(const QSharedPointer<QStringList>& tableEntries)
	{
		QString myEntry = "";
		QTableWidgetItem test;
		int entriesCount = tableEntries->length();
		this->insertRow(m_rowInd);

		//adding autoID column for first;
		if (insertROW_ID)
		{
			this->m_currentItem->setText(QString("%1").arg(this->m_autoRID));
			this->setItem(m_rowInd, 0, m_currentItem->clone());
			this->m_autoRID++;
			this->m_colInd = 1;
		}

		for (const auto &tableEntry : *tableEntries)
		{
			this->m_currentItem->setText(tableEntry);
			this->setItem(m_rowInd, this->m_colInd, m_currentItem->clone());
			this->m_colInd++;
		}

		//reset colIDx for next row
		this->m_colInd = 0;



	}


	void DataTable::setHeader(const QStringList &headerEntries) {
		this->setHorizontalHeaderLabels(headerEntries);
	}

	//! reads table entries from csv file into qtable widget
	//! @param startLine is optional (nullptr)
	bool  DataTable::readTableEntries(const QString &fName, const uint rowCount, uint colCount, const int headerNr,
		const uint StartLine, const bool readHeaders, bool insertID, QString const & encoding)
	{
		if (insertID)
		{
			this->insertROW_ID = insertID;
			//cols + 1 for AutoID
			colCount++;
		}

		QString el_line;
		QFile file(fName);

		if (!prepareFile(fName, file))
			return false;
		QTextStream in(&file);
		if (!encoding.isEmpty())
			in.setCodec(encoding.toStdString().c_str());
		//skip lines and add header to table;
		prepareHeader(this->m_currHeaderLineNr, el_line, in, readHeaders, insertID);

		//read all entries;
		readTableValues(rowCount, in, el_line);

		m_LastEncoding = in.codec()->name().toStdString().c_str();
		if (file.isOpen()) file.close();
		return true;
	}

	void DataTable::readTableValues(const uint &rowCount, QTextStream &file, QString &el_line)
	{
		int entriesCount = rowCount - 1;
		int row = 0;
		while (!file.atEnd())
		{
			if (row > entriesCount) break;
			el_line = file.readLine();
			*this->m_currentEntry = el_line.split(m_FileSeperator);
			this->addLineToTable(this->m_currentEntry);
			this->m_rowInd++;
			entriesCount;
			row++;
		}

		this->isDataFilled = true;
	}

	void DataTable::prepareHeader(uint headerLine, QString &el_line, QTextStream &file, const bool &readHeaders, bool insertID)
	{
		for (int curRow = 0; curRow < headerLine; curRow++)
		{
			el_line = file.readLine();
		}

		//nextLine is headerLine if not enabled skip is this line
		el_line = file.readLine();
		if (readHeaders && !el_line.isEmpty())
		{
			*this->m_headerEntries = el_line.split(m_FileSeperator);

			//resize table
			if (this->m_headerEntries->length() > this->m_colCount)
			{
				this->setColumnCount(this->m_headerEntries->length());
			}

			if (insertID)
			{
				//insert autoID header;
				this->m_headerEntries->insert(this->m_headerEntries->begin(), this->m_rowID);
			}
			this->setHeader(*m_headerEntries);
		}
	}

	bool DataTable::prepareFile(const QString & fName, QFile &file)
	{
		if (fName.isEmpty())
			return false;
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::information(this, tr("Unable to open file"), file.errorString());
			return false;
		}
		return true;
	}

	void DataTable::setColSeparator(const csvConfig::csvSeparator & separator)
	{
		switch (separator)
		{
			default:
			case csvConfig::csvSeparator::Colunm: m_FileSeperator = ";"; break;
			case csvConfig::csvSeparator::Comma:  m_FileSeperator = ","; break;
		}
	}

	QString DataTable::getLastEncoding() const
	{
		return m_LastEncoding;
	}
}

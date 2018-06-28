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

#include "iACsvIO.h"

#include <QFile>
#include <QMessageBox>
#include <QTextCodec>
#include <QTextStream>

DataTable::DataTable()
{
	setSortingEnabled(false);
	setShowGrid(true);
	setEnabled(false);
}

void DataTable::clearTable()
{
	clear();
	setRowCount(0);
}

void DataTable::addLineToTable(QStringList const & tableEntries, size_t row, bool addAutoID)
{
	insertRow(row);
	uint colInd = 0;
	if (addAutoID) // adding autoID column
	{
		setItem(row, colInd, new QTableWidgetItem(QString("%1").arg(row)));
		++colInd;
	}
	for (const auto &tableEntry : tableEntries)
	{
		setItem(row, colInd, new QTableWidgetItem(tableEntry));
		++colInd;
	}
}

bool  DataTable::readTableEntries(const QString &fName, const uint rowCount, const QString & colSeparator,
	const uint skipLinesStart, const bool readHeaders, bool addAutoID, QString const & encoding)
{
	if (fName.isEmpty())
		return false;
	QFile file(fName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(this, tr("FeatureScout"), tr("Unable to open file: %1").arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
	if (!encoding.isEmpty())
		in.setCodec(encoding.toStdString().c_str());
		
	//skip lines and add header to table;
	prepareHeader(skipLinesStart, in, readHeaders, addAutoID, colSeparator);
	readTableValues(rowCount, in, addAutoID, colSeparator);

	m_LastEncoding = in.codec()->name().toStdString().c_str();
	if (file.isOpen()) file.close();
	return true;
}

void DataTable::readTableValues(size_t const rowCount, QTextStream &file, bool addAutoID, const QString & colSeparator)
{
	size_t row = 0;
	while (!file.atEnd() && row < rowCount)
	{
		QString el_line = file.readLine();
		auto currentEntry = el_line.split(colSeparator);
		addLineToTable(currentEntry, row, addAutoID);
		++row;
	}
	setRowCount(row);
}

void DataTable::prepareHeader(uint skipLinesStart, QTextStream &file, bool readHeaders, bool addAutoID, const QString & colSeparator)
{
	for (int curRow = 0; curRow < skipLinesStart; curRow++)
		file.readLine();

	QString line = file.readLine();	// header line
	if (readHeaders)
	{
		m_headerEntries = line.split(colSeparator);
		if (addAutoID)
			m_headerEntries.insert(m_headerEntries.begin(), iACsvIO::ColNameAutoID);
		setColumnCount(m_headerEntries.length());
		setHorizontalHeaderLabels(m_headerEntries);
	}
}

QString DataTable::getLastEncoding() const
{
	return m_LastEncoding;
}

const QStringList & DataTable::getHeaders() const
{
	return m_headerEntries;
}

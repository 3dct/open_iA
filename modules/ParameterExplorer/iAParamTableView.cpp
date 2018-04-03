/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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
#include "iAParamTableView.h"

#include "iAConsole.h"

#include <QFile>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTextStream>

iAParamTableView::iAParamTableView(QString const & csvFileName):
	m_table(new QTableWidget())
{
	LoadCSVData(csvFileName);
	QHBoxLayout* lay = new QHBoxLayout();
	lay->addWidget(m_table);
	setLayout(lay);
}

void iAParamTableView::LoadCSVData(QString const & csvFileName)
{
	QFile file(csvFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Could not read csv file %1").arg(csvFileName));
		return;
	}
	QStringList csvLines;
	QTextStream textStream(&file);
	while (!textStream.atEnd())
		csvLines << textStream.readLine();
	file.close();

	m_table->clear();
	m_table->setRowCount(csvLines.size());

	QStringList headers = csvLines[0].split(",");
	m_table->setColumnCount(headers.size());
	//for (int c  = 0; c < headers.size(); ++c)
	//	m_table->setHorizontalHeaderItem(c, new QTableWidgetItem(headers[c]));

	m_columnBounds.clear();
	m_columnBounds.fill(qMakePair(std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()), headers.size());
	for (int row=0; row<csvLines.size(); ++row)
	{
		QStringList items = csvLines[row].split(",");	// TODO: consider quoted strings?
		//if (items.size() > m_table->columnCount())
		//	m_table->setColumnCount(items.size());
		for (int col = 0; col < items.size(); ++col)
		{
			m_table->setItem(row, col, new QTableWidgetItem(items[col]));
			bool ok;
			double val = items[col].toDouble(&ok);
			if (ok)
			{
				if (val < m_columnBounds[col].first)
					m_columnBounds[col].first = val;
				if (val > m_columnBounds[col].second)
					m_columnBounds[col].second = val;
			}
		}
	}
}

double iAParamTableView::ColumnMin(int col) const
{
	return m_columnBounds[col].first;
}

double iAParamTableView::ColumnMax(int col) const
{
	return m_columnBounds[col].second;
}

QTableWidget* iAParamTableView::Table()
{
	return m_table;
}

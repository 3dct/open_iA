/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAParamTableView.h"

#include <iALog.h>

#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
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
		LOG(lvlInfo, QString("Could not read csv file %1").arg(csvFileName));
		return;
	}
	QStringList csvLines;
	QTextStream textStream(&file);
	while (!textStream.atEnd())
		csvLines << textStream.readLine();
	file.close();

	m_table->clear();
	m_table->setRowCount(csvLines.size());
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setVisible(false);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	QStringList headers = csvLines[0].split(",");
	m_table->setColumnCount(headers.size());
	//for (int c  = 0; c < headers.size(); ++c)
	//	m_table->setHorizontalHeaderItem(c, new QTableWidgetItem(headers[c]));

	m_columnBounds.clear();
	m_columnBounds.fill(qMakePair(std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()), headers.size());
	for (int row=0; row<csvLines.size(); ++row)
	{
		QStringList items = csvLines[row].split(",");	// TODO: consider quoted strings?
		if (items.size() <= 1)
		{
			items = csvLines[row].split(";");
		}
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
		if (items.size() < headers.size())
		{
			LOG(lvlInfo, QString("Line %1 has less columns(%2) than expected(%3)").arg(row).arg(items.size()).arg(headers.size()));
			for (int col = items.size(); col < headers.size(); ++col)
			{
				m_table->setItem(row, col, new QTableWidgetItem("0"));
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

void iAParamTableView::ShowFeature(int index, bool show)
{
	if (index == 0)
		return;
	m_table->setColumnHidden(index, !show);
}

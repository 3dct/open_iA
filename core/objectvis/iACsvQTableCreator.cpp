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
#include "iACsvQTableCreator.h"

#include <iALog.h>

#include <QTableWidget>

iACsvQTableCreator::iACsvQTableCreator(QTableWidget* tblWidget) :
	m_table(tblWidget)
{}

void iACsvQTableCreator::initialize(QStringList const & headers, size_t const rowCount)
{
	if (rowCount > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("iACsvQTableCreator: More rows (%1) than I can handle (%2)").arg(rowCount).arg(std::numeric_limits<int>::max()));
	}
	m_table->setColumnCount(headers.size());
	m_table->setRowCount(static_cast<int>(rowCount));
	m_table->setHorizontalHeaderLabels(headers);
}

void iACsvQTableCreator::addRow(size_t row, QStringList const & values)
{
	uint col = 0;
	for (const auto &value : values)
	{                   // we made sure in initialize(...) that rowCount < int_max
		m_table->setItem(static_cast<int>(row), col, new QTableWidgetItem(value));
		++col;
	}
}

QTableWidget* iACsvQTableCreator::table()
{
	return m_table;
}

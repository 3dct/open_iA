// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	m_table->setColumnCount(static_cast<int>(headers.size()));
	m_table->setRowCount(static_cast<int>(rowCount));
	m_table->setHorizontalHeaderLabels(headers);
}

void iACsvQTableCreator::addRow(size_t row, std::vector<double> const & values)
{
	uint col = 0;
	for (const auto &value : values)
	{                   // we made sure in initialize(...) that rowCount < int_max
		m_table->setItem(static_cast<int>(row), col, new QTableWidgetItem(QString::number(value, 'f', 10)));
		++col;
	}
}

QTableWidget* iACsvQTableCreator::table()
{
	return m_table;
}

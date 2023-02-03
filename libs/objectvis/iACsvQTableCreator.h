// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvIO.h"

#include "iAobjectvis_export.h"

class QTableWidget;

class iAobjectvis_API iACsvQTableCreator : public iACsvTableCreator
{
public:
	iACsvQTableCreator(QTableWidget* tblWidget);
	void initialize(QStringList const & headers, size_t const rowCount) override;
	void addRow(size_t row, std::vector<double> const & values) override;
	QTableWidget* table();
private:
	QTableWidget* m_table;
};

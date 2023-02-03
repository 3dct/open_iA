// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvIO.h"

#include "iAobjectvis_export.h"

#include <vector>

class iAobjectvis_API iACsvVectorTableCreator : public iACsvTableCreator
{
public:
	using ValueType = double;
	using TableType = std::vector<std::vector<ValueType>>;
	iACsvVectorTableCreator();
	void initialize(QStringList const & headers, size_t const rowCount) override;
	void addRow(size_t row, std::vector<double> const & values) override;
	TableType const & table();
	QStringList const& header();
private:
	QStringList m_header;
	TableType m_values;   //!< output values
};

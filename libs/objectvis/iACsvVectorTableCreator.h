// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvIO.h"

#include "iAobjectvis_export.h"

#include <vector>

//! Fills a vector-based table with values from a .csv file.
//! To be used in conjunction with iACsvIO::loadCSV; creates an std::vector of std::vectors of type double
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
	QStringList m_header; //!< column names
	TableType m_values;   //!< output values
};

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvIO.h"

#include "iAobjectvis_export.h"

#include <vtkSmartPointer.h>

class vtkTable;

//! Fills a vtkTable with values from a .csv file.
//! To be used in conjunction with iACsvIO::loadCSV
class iAobjectvis_API iACsvVtkTableCreator : public iACsvTableCreator
{
public:
	iACsvVtkTableCreator();
	void initialize(QStringList const & headers, size_t const rowCount) override;
	void addRow(size_t row, std::vector<double> const & values) override;
	vtkSmartPointer<vtkTable> table();
private:
	vtkSmartPointer<vtkTable> m_table;   //!< output vtk table
	//void debugTable(const bool useTabSeparator); //! <debugTable)
};

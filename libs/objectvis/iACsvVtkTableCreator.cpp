// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACsvVtkTableCreator.h"

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkTable.h>

iACsvVtkTableCreator::iACsvVtkTableCreator()
	: m_table(vtkSmartPointer<vtkTable>::New())
{}

void iACsvVtkTableCreator::initialize(QStringList const & headers, size_t const rowCount)
{
	m_table->Initialize();
	// ID column (int):
	auto arrID = vtkSmartPointer<vtkIntArray>::New();
	arrID->SetName(headers[0].toUtf8().constData());
	m_table->AddColumn(arrID);
	// other columns (float):
	for (int col = 1; col < headers.size() - 1; ++col)
	{
		auto arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName(headers[col].toUtf8().constData());
		m_table->AddColumn(arrX);
	}
	// class column (int):
	auto arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(headers[headers.size() - 1].toUtf8().constData());
	m_table->AddColumn(arr);

	m_table->SetNumberOfRows(rowCount);
}

void iACsvVtkTableCreator::addRow(size_t row, std::vector<double> const & values)
{
	m_table->SetValue(row, 0, static_cast<int>(values[0])); // ID
	for (size_t col = 1; col < values.size() - 1; ++col)
	{
		m_table->SetValue(row, col, values[col]);
	}
	m_table->SetValue(row, values.size() - 1, values[values.size() - 1]); // class
}

vtkSmartPointer<vtkTable> iACsvVtkTableCreator::table()
{
	return m_table;
}


//void iACsvVtkTableCreator::debugTable(const bool useTabSeparator)
//{
//	std::string separator = (useTabSeparator) ? "\t" : ",";
//	std::ofstream debugfile;
//	debugfile.open("C:/Users/p41883/Desktop/inputData.txt");
//	if (debugfile.is_open())
//	{
//		vtkVariant spCol, spRow, spCN, spVal;
//		spCol = m_table->GetNumberOfColumns();
//		spRow = m_table->GetNumberOfRows();
//
//		for (int i = 0; i<spCol.ToInt(); i++)
//		{
//			spCN = m_table->GetColumnName(i);
//			debugfile << spCN.ToString() << separator;
//		}
//		debugfile << "\n";
//		for (int row = 0; row < spRow.ToInt(); row++)
//		{
//			for (int col = 0; col < spCol.ToInt(); col++)
//			{
//				spVal = m_table->GetValue(row, col);
//				debugfile << spVal.ToString() << separator; //TODO cast debug to double
//			}
//			debugfile << "\n";
//		}
//		debugfile.close();
//	}
//}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACsvVectorTableCreator.h"


iACsvVectorTableCreator::iACsvVectorTableCreator()
{}

void iACsvVectorTableCreator::initialize(QStringList const & headers, size_t const rowCount)
{
	m_header = headers;
	for (int col = 0; col < headers.size(); ++col)
	{
		m_values.push_back(std::vector<double>(rowCount, 0));
	}
}

void iACsvVectorTableCreator::addRow(size_t row, std::vector<double> const & values)
{
	for (size_t col = 0; col < values.size(); ++col)
	{
		m_values[col][row] = values[col];
	}
}

iACsvVectorTableCreator::TableType const& iACsvVectorTableCreator::table()
{
	return m_values;
}

QStringList const& iACsvVectorTableCreator::header()
{
	return m_header;
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

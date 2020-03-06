/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

void iACsvVectorTableCreator::addRow(size_t row, QStringList const & values)
{
	for (int col = 0; col < values.size(); ++col)
	{
		m_values[col][row] = values[col].toDouble();
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
//	ofstream debugfile;
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

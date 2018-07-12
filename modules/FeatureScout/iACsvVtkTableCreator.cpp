/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
	vtkSmartPointer<vtkIntArray> arrID = vtkSmartPointer<vtkIntArray>::New();
	arrID->SetName(headers[0].toUtf8().constData());
	m_table->AddColumn(arrID);
	// other columns (float):
	for (int col = 1; col < headers.size() - 1; ++col)
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName(headers[col].toUtf8().constData());
		m_table->AddColumn(arrX);
	}
	// class column (int):
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(headers[headers.size() - 1].toUtf8().constData());
	m_table->AddColumn(arr);

	m_table->SetNumberOfRows(rowCount);
}

void iACsvVtkTableCreator::addRow(size_t row, QStringList const & values)
{
	m_table->SetValue(row, 0, values[0].toInt()); // ID
	for (int col = 1; col < values.size() - 1; ++col)
		m_table->SetValue(row, col, values[col].toFloat());
	m_table->SetValue(row, values.size() - 1, values[values.size() - 1].toFloat()); // class
}

vtkSmartPointer<vtkTable> iACsvVtkTableCreator::getTable()
{
	return m_table;
}

/*
void iACsvVtkTableCreator::debugTable(const bool useTabSeparator)
{
	std::string separator = (useTabSeparator) ? "\t" : ",";
	ofstream debugfile;
	debugfile.open("C:/Users/p41883/Desktop/inputData.txt");
	if (debugfile.is_open())
	{
		vtkVariant spCol, spRow, spCN, spVal;
		spCol = m_table->GetNumberOfColumns();
		spRow = m_table->GetNumberOfRows();

		for (int i = 0; i<spCol.ToInt(); i++)
		{
			spCN = m_table->GetColumnName(i);
			debugfile << spCN.ToString() << separator;
		}
		debugfile << "\n";
		for (int row = 0; row < spRow.ToInt(); row++)
		{
			for (int col = 0; col < spCol.ToInt(); col++)
			{
				spVal = m_table->GetValue(row, col);
				debugfile << spVal.ToString() << separator; //TODO cast debug to double
			}
			debugfile << "\n";
		}
		debugfile.close();
	}
}
*/
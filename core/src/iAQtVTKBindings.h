/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include <QTableWidget>

#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkDoubleArray.h>
#include <vtkNew.h>

inline vtkSmartPointer<vtkTable> convertQTableWidgetToVTKTable( const QTableWidget * data )
{
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();

	int columnNb = data->columnCount();
	int rowNb = data->rowCount() - 1;

	// set table header
	for( int i = 0; i < columnNb; ++i )
	{
		QString columnName = data->item( 0, i )->text();
		vtkNew<vtkDoubleArray> array;
		array->SetName( columnName.toStdString().c_str() );
		table->AddColumn( array.GetPointer() );
	}

	// set table rows
	table->SetNumberOfRows( rowNb );

	// set table values
	for( int i = 0; i < rowNb; ++i )
		for( int j = 0; j < columnNb; ++j )
			table->SetValue( i, j, data->item( i + 1, j )->text().toDouble() ); //1 - to skip header

	return table;
}

// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

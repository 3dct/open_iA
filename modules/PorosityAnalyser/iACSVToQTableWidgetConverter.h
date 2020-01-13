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

#include <QString>
#include <QTableWidget>
#include <QFile>
#include <QTextStream>

namespace iACSVToQTableWidgetConverter
{
	inline void loadCSVFile(QString csvFile, QTableWidget * tableWidget)
	{
		QFile f( csvFile );

		if( !f.open( QIODevice::ReadOnly ) )
			return;

		QTextStream ts( &f );
		QList< QStringList > list;
		int row = 0, col = 0;

		// read entire file and parse lines into list of stringlist's
		while (!ts.atEnd())
		{
			list << ts.readLine().split(",");
		}

		f.close();  // done with file

		tableWidget->setRowCount( list.count() );
		int columnCount = 0;
		for (QStringList l : list)
		{
			if (l.count() > columnCount)
			{
				columnCount = l.count();
			}
		}
		tableWidget->setColumnCount( columnCount );
		tableWidget->setUpdatesEnabled( false );  // for faster processing of large lists
		for (QStringList l: list)
		{
			for (QString str : l)
			{
				tableWidget->setItem(row, col++, new QTableWidgetItem(str));
			}
			row++; col = 0;
		}
		tableWidget->setUpdatesEnabled( true );  // done with load
	}

	inline int getCSVFileColumnCount( const QString & csvFile )
	{
		QFile f( csvFile );

		if (!f.open(QIODevice::ReadOnly))
		{
			return -1;
		}

		QTextStream ts( &f );
		QList< QStringList > list;

		// read entire file and parse lines into list of stringlist's
		while (!ts.atEnd())
		{
			list << ts.readLine().split(",");
		}

		f.close();  // done with file

		int columnCount = 0;
		for (QStringList l : list)
		{
			if (l.count() > columnCount)
			{
				columnCount = l.count();
			}
		}
		return columnCount;
	}

	inline void saveToCSVFile(QTableWidget & tableWidget, QString csvFile)
	{
		QFile f( csvFile );

		if (f.open(QIODevice::WriteOnly))
		{
			QTextStream ts(&f);
			QStringList strList;

			for (int r = 0; r < tableWidget.rowCount(); ++r)
			{
				strList.clear();
				for (int c = 0; c < tableWidget.columnCount(); ++c)
				{
					if (tableWidget.item(r, c))
					{
						strList << tableWidget.item(r, c)->text();
					}
				}
				ts << strList.join(",") + "\n";
			}
			f.close();
		}
	}

}//namespace iACSVToQTableWidgetConverter

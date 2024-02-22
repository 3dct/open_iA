// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

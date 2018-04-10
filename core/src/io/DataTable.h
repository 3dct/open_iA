#pragma once

#include "open_iA_Core_export.h"
#include "io/csv_config.h"
#include <qstringlist.h>
#include <qtablewidget.h>
#include <QSharedPointer>
#include <qstringlist.h>
#include <qstring.h>


class QFile; 


namespace  DataIO{
			

	class open_iA_Core_API DataTable : public QTableWidget
	{
	public:
		DataTable();
		void initToDefault();
		~DataTable();

		

		void initTable();

		inline const DataTable &getPreviewTable() const {
			return *this;
		}

		void addLineToTable(const QSharedPointer<QStringList> &tableEntries);
		
		//reading rows from a file; 
		bool readTableEntries(const QString &fName, const uint rowCount, uint colCount, const int headerNr, const uint *StartLine, const bool readHeaders, bool insertID);

		void readTableValues(const uint &rowCount, QFile &file, QString &el_line);

		void prepareHeader(int headerLine, QString &el_line, QFile &file, const bool &readHeaders, bool insertID);

		bool prepareFile(const QString & fName, QFile &file, bool &retflag);

		void prepareTable(const int rowCount, const int colCount, const int headerLineNr);

		void clearTable();
		void resetIndizes();


		inline void showTable() {
			this->show();
		}

		//writing all elements to file
		void dataToFile(const QString &FileName);


		void setHeader(const QStringList &headerEntries);
		void setColSeparator(const csvConfig::csvSeparator & separator);

		inline const QStringList &getHeaders() {
			return *this->m_headerEntries; 
		}

	protected:
		QSharedPointer<QStringList> m_currentEntry;
		
		QSharedPointer<QStringList> m_headerEntries; 

		QTableWidgetItem *m_currentItem;
		QItemSelectionModel *m_variableModel;
		QModelIndex m_item; 
		uint m_rowInd;
		uint m_colInd;
		uint m_currHeaderLineNr; 
		bool isInitialized;
		bool isDataFilled;

		int m_colCount;

		//insert auto row ID
		bool insertROW_ID;
		QString m_FileSeperator; 
		QString m_FileName; 

		//name of first column
		QString m_rowID;

		//row ID automatically assigned
		uint m_autoRID; 
	private:
		//disable copy constructor
		DataTable(const DataTable &other);

	};

	
}

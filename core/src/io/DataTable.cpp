#include "DataTable.h"
#include <qfile.h>
#include <qmessagebox.h>

namespace DataIO {

	DataTable::DataTable()
	{
		/*this->QTableWidget(); */
		this->m_currentItem = 0;
		this->m_currentEntry = 0;
		//this->m_currentString = 0; 
		this->m_variableModel = 0;
		this->m_currHeaderLineNr = 0; 
		this->m_colInd = 0; 
		this->m_rowInd = 0; 
		this->m_currHeaderLineNr = 0; 
		this->m_headerEntries = 0; 
		this->m_FileSeperator = ";";

		
	}


	DataTable::~DataTable()
	{
		if (!this->m_currentItem) {
			delete this->m_currentItem;
			this->m_currentItem = 0;
		}

		this->m_variableModel = 0;
	}

	void DataTable::initTable()
	{
		if (!this->m_currentItem) {
			this->m_currentItem = new QTableWidgetItem();
		}

		if (!this->m_currentEntry) {
			this->m_currentEntry = QSharedPointer<QStringList>(new QStringList());
		}

		if (!this->m_variableModel) {
			this->m_variableModel = this->selectionModel();
		}

		if (!this->m_headerEntries) {
			this->m_headerEntries = QSharedPointer<QStringList>(new QStringList()); 
		}
		
	}

	void DataTable::prepareTable(const int rowCount, const int colCount, const int headerLineNr) {
		this->initTable(); 
		this->setRowCount((int)rowCount);
		this->setColumnCount((int)colCount);
		this->setSortingEnabled(false);
		this->setShowGrid(true);
		this->setEnabled(false); 
		this->m_currHeaderLineNr = headerLineNr;
	}


	void DataTable::addLineToTable(const QSharedPointer<QStringList>& tableEntries)
	{
		QString myEntry = "";
		QTableWidgetItem test; 
		int entriesCount = tableEntries->length();
		this->insertRow(m_rowInd); 

			for (const auto &tableEntry : *tableEntries) {
				this->m_currentItem->setText(tableEntry);
				this->setItem(m_rowInd, this->m_colInd, m_currentItem->clone());
				this->m_colInd++;
			}

			this->m_rowInd++; 

			//reset colIDx for next row
			this->m_colInd = 0; 


			
	}


	void DataTable::setHeader(const QStringList &headerEntries) {
		this->setHorizontalHeaderLabels(headerEntries);
	}

	//reads table entries from csv file into qtable widget
	//optional startLine as nullptr
	bool  DataTable::readTableEntries(const QString &fName, const uint rowCount, const uint colCount,const int headerNr,  const uint *StartLine, const bool readHeaders)
	{
		this->prepareTable(rowCount, colCount, headerNr );

		int startRow = -1; 

		//number of rows to skip
		if (StartLine ) {
			//startRow Line where the header starts
			startRow = (*StartLine)-1;
		}
		QFile file(fName);
		QString el_line; 
		//QString file_separator; 
		

		if (fName.isEmpty()) {
			return false;
		}
		
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information(this, tr("Unable to open file"),
				file.errorString());
			return false; 
		
		}

		int headerLine = (uint) this->m_currHeaderLineNr -1;
		int row = 0; 
		
		if (readHeaders) {
			

			for (row; row < m_currHeaderLineNr; row++) {
				el_line = file.readLine();

				if (row == headerLine)
				{
					*this->m_headerEntries = el_line.split(m_FileSeperator);
					this->setHeader(*m_headerEntries);
					break; 
				}

			}
			
		
		
		}

		int rCount = rowCount + *StartLine - 1;
		
		for (row; row < rCount; row++) {
			el_line = file.readLine();

			if (row > headerLine-1)
			{
				
					*this->m_currentEntry = el_line.split(m_FileSeperator);
					this->addLineToTable(this->m_currentEntry);
			}
			
		}

		return true; 
	}

	void DataTable::setColSeparator(const csvConfig::csvSeparator & separator) {
		switch (separator)
		{
		case csvConfig::csvSeparator::Colunm:
			m_FileSeperator = ";";
			break;
		case csvConfig::csvSeparator::Comma:
			m_FileSeperator = ",";
			break;

		default:m_FileSeperator = ";";
			break;
		}
	
	}
}
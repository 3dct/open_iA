#include "DataTable.h"
#include <qfile.h>
#include <qmessagebox.h>

namespace DataIO {

	DataTable::DataTable()
	{
		/*this->QTableWidget(); */
		initToDefault();
	}

	void DataTable::initToDefault()
	{
		this->m_currentItem = 0;
		this->m_currentEntry = 0;
		//this->m_currentString = 0; 
		this->m_variableModel = 0;
		this->m_currHeaderLineNr = 0;
		this->m_colInd = 0;
		this->m_rowInd = 0;
		this->m_currHeaderLineNr = 0;
		this->m_headerEntries = 0;
		this->m_FileSeperator = ",";
		this->isDataFilled = false; 
		this->m_autoRID = 0;
		this->m_rowID = "AUTO_ID";
		this->insertROW_ID = false; 

		//minimum to values to show
		this->m_colCount = 31; 
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
		//this->initToDefault(); 
		this->initTable(); 
		this->setRowCount((int)rowCount);
		this->setColumnCount((int)colCount);
		this->setSortingEnabled(false);
		this->setShowGrid(true);
		this->setEnabled(false); 
		this->m_currHeaderLineNr = headerLineNr;
	}


	//to clear the table when adding new entries 
	void DataTable::clearTable()
	{
		if (isDataFilled) {
			this->clear();
			this->model()->removeRows(0, this->rowCount());
			resetIndizes();
			isDataFilled = false; 
		} 
		
	}

	void DataTable::resetIndizes()
	{
		this->m_rowInd = 0;
		this->m_colInd = 0;
	}

	//adding file entry to table + first column is auto id; 
	void DataTable::addLineToTable(const QSharedPointer<QStringList>& tableEntries)
	{
		QString myEntry = "";
		QTableWidgetItem test; 
		int entriesCount = tableEntries->length();
		this->insertRow(m_rowInd); 
		
		//adding autoID column for first;
		if (insertROW_ID) {
			this->m_currentItem->setText(QString("%1").arg(this->m_autoRID));
			this->setItem(m_rowInd, 0, m_currentItem->clone());
			this->m_autoRID++; 
			this->m_colInd = 1; 
		}

		
		for (const auto &tableEntry : *tableEntries) {
				this->m_currentItem->setText(tableEntry);
				this->setItem(m_rowInd, this->m_colInd, m_currentItem->clone());
				this->m_colInd++;
		}

		//reset colIDx for next row
		this->m_colInd = 0; 


			
	}


	void DataTable::setHeader(const QStringList &headerEntries) {
		this->setHorizontalHeaderLabels(headerEntries);
	}

	//reads table entries from csv file into qtable widget
	//optional startLine as nullptr
	bool  DataTable::readTableEntries(const QString &fName, const uint rowCount, uint colCount,const int headerNr,  const uint *StartLine, const bool readHeaders, bool insertID)
	{ 
		if (insertID) {
			this->insertROW_ID = insertID;

			//cols + 1 for AutoID 
			colCount++; 
		}
	
		//this->prepareTable(rowCount, colCount, headerNr );
		int startRow = -1; 
		int headerLine = (uint) this->m_currHeaderLineNr -1;
		QString el_line; 
		QFile file(fName);
		//number of rows to skip
		if (StartLine ) {
			//startRow Line where the header starts
			startRow = (*StartLine)-1;
		}
		
		bool retflag;
		bool retval = prepareFile(fName, file, retflag);
		if (retflag) return retval;

		//skip lines and add header to table;
		prepareHeader(headerLine, el_line, file, readHeaders, insertID);
		
		//read all entries; 
		readTableValues(rowCount, file, el_line);
		return true; 
	}

	void DataTable::readTableValues(const uint &rowCount, QFile &file, QString &el_line)
	{
		int entriesCount = rowCount - 1;
		int row = 0;
		while (!file.atEnd())
		{
			if (row > entriesCount) break;
			el_line = file.readLine();
			*this->m_currentEntry = el_line.split(m_FileSeperator);
			this->addLineToTable(this->m_currentEntry);
			this->m_rowInd++;
			entriesCount;
			row++;
		}

		this->isDataFilled = true;

		if (file.isOpen()) file.close();
	}

	void DataTable::prepareHeader(int headerLine, QString &el_line, QFile &file, const bool &readHeaders, bool insertID)
	{
		for (int curRow = 0; curRow < headerLine; curRow++) {
			el_line = file.readLine();
		}


		//nextLine is headerLine if not enabled skip is this line
		el_line = file.readLine();
		if (readHeaders) {
			if (!el_line.isEmpty()) {
				
				*this->m_headerEntries = el_line.split(m_FileSeperator);
				
				//resize table
				if(this->m_headerEntries->length() > this->m_colCount){
					this->setColumnCount(this->m_headerEntries->length()); 
				}

				if (insertID) {
					//insert autoID header; 
					this->m_headerEntries->insert(this->m_headerEntries->begin(), this->m_rowID);
				}

				this->setHeader(*m_headerEntries);
			}
		}


	}

	bool DataTable::prepareFile(const QString & fName, QFile &file, bool &retflag)
	{
		retflag = true;
		if (fName.isEmpty()) {
			return false;
		}

		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information
			(this, tr("Unable to open file"), file.errorString());
			return false;
		}

		retflag = false;
		return {};
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
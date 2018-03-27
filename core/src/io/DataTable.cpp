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

		this->m_autoRID = 0; 
		this->m_rowID = "AUTO_ID";
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


	//adding file entry to table + first column is auto id; 
	void DataTable::addLineToTable(const QSharedPointer<QStringList>& tableEntries)
	{
		QString myEntry = "";
		QTableWidgetItem test; 
		int entriesCount = tableEntries->length();
		this->insertRow(m_rowInd); 
		
		//adding autoID column for first;
		this->m_currentItem->setText(QString("%1").arg(this->m_autoRID));
		this->setItem(m_rowInd, 0, m_currentItem->clone());
		this->m_autoRID++;
		this->m_colInd = 1; 
		

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

		//cols + 1 for AutoID
		this->prepareTable(rowCount, colCount+1, headerNr );
		int startRow = -1; 

		//number of rows to skip
		if (StartLine ) {
			//startRow Line where the header starts
			startRow = (*StartLine)-1;
		}
		QFile file(fName);
		QString el_line; 


		if (fName.isEmpty()) {
			return false;
		}
		
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::information
			(this, tr("Unable to open file"),file.errorString());
			return false; 
		
		}

		int headerLine = (uint) this->m_currHeaderLineNr -1;
		



		//skip lines;
		for (int curRow = 0; curRow < headerLine; curRow++) {
			el_line = file.readLine(); 
		}


		//nextLine is headerLine if not enabled skip is this line
		el_line = file.readLine(); 
		if (readHeaders) {
			if (!el_line.isEmpty()) {
				*this->m_headerEntries = el_line.split(m_FileSeperator);

				//insert autoID header; 
				this->m_headerEntries->insert(this->m_headerEntries->begin(), this->m_rowID);
				this->setHeader(*m_headerEntries);
			}
		} 



		//read all entries; 
		int entriesCount = rowCount-1; 
		int row = 0; 
		while (!file.atEnd())
		{
			if (row > entriesCount) break; 
			
			el_line = file.readLine();
			*this->m_currentEntry = el_line.split(m_FileSeperator);
			this->addLineToTable(this->m_currentEntry);
			entriesCount;
			row++;
			
		}

		

		if (!file.isOpen()) file.close(); 
		return true; 
	}

	//reads all with selected cols; 
	/*bool readAllEntries() {


		return true; 
	}*/

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
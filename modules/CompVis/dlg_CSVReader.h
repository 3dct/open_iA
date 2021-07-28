#pragma once

#include "ui_CSVReader.h"

#include "iACsvDataStorage.h"

//TODO 
	//all datasets must have the same header/ the same characteristics
	//all values in all datasets have to be positive --> [0, Infinity[ !!

class dlg_CSVReader : public QDialog, public Ui_CSVReader
{
	Q_OBJECT
   public:
	//! Create a new dialog
	dlg_CSVReader(QWidget* parent = 0, Qt::WindowFlags f = 0);

	iACsvDataStorage* getCsvDataStorage();
	
   private slots:
	//! On button click for selecting one/several CSV files
	void btnAddFilesClicked();
	//! On button click for deleting specific CSV file
	void btnDeleteFileClicked();
	//! handles a click on the OK button
	void okBtnClicked();

   private:
	//! connect signals and slots of all dialog controls
	void connectSignals();

	QString m_path;
	QStringList m_filenames;
	iACsvDataStorage* m_dataStorage;

};
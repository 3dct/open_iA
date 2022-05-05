/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "ui_CSVReader.h"
#include "iACsvDataStorage.h"
#include "dlg_CSVInput.h"

//TODO 
	//all datasets must have the same header/ the same characteristics
	//all values in all datasets have to be positive --> [0, Infinity[ !!

class dlg_CSVReader : public QDialog, public Ui_CSVReader  //public dlg_CSVInput
{
	Q_OBJECT
   public:
	//! Create a new dialog
	dlg_CSVReader();
	   
	iACsvDataStorage* getCsvDataStorage();

   private slots:
	//! On button click for selecting one/several CSV files
	void btnAddFilesClicked();
	//! On button click for deleting specific CSV file
	void btnDeleteFileClicked();
	//! handles a click on the OK button
	void okBtnClicked();

	void checkMDS();

   private:
	//! connect signals and slots of all dialog controls
	void connectSignals();

	//! Assign the input object type from the GUI to the internal configuration object
	void assignObjectTypes();

	QString m_path;
	QStringList m_filenames;
	iACsvDataStorage* m_dataStorage;
};
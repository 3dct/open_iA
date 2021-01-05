/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAcore_export.h"

#include "ui_FilterSelection.h"
#include "qthelper/iAQTtoUIConnector.h"


typedef iAQTtoUIConnector<QDialog, Ui_FilterSelectionDlg> iAFilterSelectionConnector;

//! Dialog for selecting a filter from the ones currently registered with the iAFilterRegistry.
class iAcore_API iAFilterSelectionDlg : public iAFilterSelectionConnector
{
Q_OBJECT
public:
	//! Create a new filter selection dialog.
	//! @param parent the parent widget of the dialog (dialog will be a top level window if you pass nullptr)
	//! @param preselectedFilter the name of a filter that should be preselected in the dialog
	iAFilterSelectionDlg(QWidget * parent, QString const & preselectedFilter = "");
	//! Retrieve the name of the filter that the user has selected.
	//! @return a filter name as it can be passed to iAFilterRegistry::filter() for creating an instance of the filter.
	QString selectedFilterName() const;
public slots:
	//! Called when the user has changed the content of the input field on top of the dialog.
	//! Causes the list of filters to be updated to only show the ones
	//! where at least a part of the name matches what the user has entered.
	void filterChanged(QString const &);
	//! Called when the user selects a filter from the list.
	void listSelectionChanged(QListWidgetItem *, QListWidgetItem *);
private:
	//! check whether one (shown) item is selected, and if it is, show its description and enable OK button.
	void updateOKAndDescription();
	int m_curMatches;
};
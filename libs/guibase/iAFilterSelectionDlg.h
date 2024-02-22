// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QDialog>

#include <memory>

class Ui_FilterSelectionDlg;

class QListWidgetItem;

//! Dialog for selecting a filter from the ones currently registered with the iAFilterRegistry.
class iAguibase_API iAFilterSelectionDlg : public QDialog
{
Q_OBJECT
public:
	//! Create a new filter selection dialog.
	//! @param parent the parent widget of the dialog (dialog will be a top level window if you pass nullptr)
	//! @param preselectedFilter the name of a filter that should be preselected in the dialog
	iAFilterSelectionDlg(QWidget * parent, QString const & preselectedFilter = "");
	~iAFilterSelectionDlg();	// required to enable forward-declaring unique_ptr...
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
	const std::unique_ptr<Ui_FilterSelectionDlg> m_ui;
};

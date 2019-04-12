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

#include "open_iA_Core_export.h"
#include "ui_CommonInput.h"

#include <QDialog>
#include <QVector>

class MainWindow;
class MdiChild;
class QWidget;
class QErrorMessage;
class QLabel;
class QScrollArea;
class QString;
class QStringList;
class QTextDocument;

//! Dialog asking the user for some given parameters.
class open_iA_Core_API dlg_commoninput : public QDialog, public Ui_CommonInput
{
	Q_OBJECT
public:
	//! Create dialog with the given parameters.
	//! @param parent the parent widget
	//! @param title  the dialog title
	//! @param labels the list of parameter labels; each name needs to have a prefix signifying the type of control used to ask for it:
	//!        $ ... QCheckBox
	//!        . ... Filter parameters (special) - line edit
	//!        # ... a line edit
	//!        + ... a combo box (in values, give a QStringList with all options, prefix the one that should be selected with a '!')
	//!        * ... a SpinBox, asking for an (integer) number between 0 and 65536
	//!        ^ ... a double SpinBox asking for a floating point number between -999999, 999999
    //!        = ... a PlainTextEdit (allowing to enter multiple lines of text)
	//!        & ... Filter type (special) - PushButton, which on click enables user to select a filter name
	//!        < ... file chooser (single file name for opening)
	//!        { ... file chooser (multiple file names for opening)
	//!        > ... file chooser (single file name for saving)
    //!        ; ... folder chooser (single existing folder name)
	//!    e.g. "*Name" gives you an (integer) spinbox with label "Name"
	//! @param values starting values for all parameters.
	//! @param fDescr an optional description text, displayed on top of the dialog
	dlg_commoninput ( QWidget *parent, QString const & title, QStringList const & labels, QList<QVariant> const & values, QTextDocument *fDescr = nullptr);
	int getCheckValue(int index) const;
	QString getComboBoxValue(int index) const;
	int getComboBoxIndex(int index) const;
	QString getText(int index) const;
	int getIntValue(int index) const;
	double getDblValue(int index) const;
	void showROI();
	int exec() override;
	void setSourceMdi(MdiChild* child, MainWindow* mainWnd);
private:
	QWidget * m_container;
	int m_roi[6];
	QVector<int> m_filterWithParameters;
	MdiChild * m_sourceMdiChild;
	MainWindow * m_mainWnd;
	bool m_sourceMdiChildClosed;
	void updateValues(QList<QVariant>);
	void UpdateROIPart(QString const & partName, QString const & value);
private slots:
	void ROIUpdated(QString text);
	void SourceChildClosed();
	void SelectFilter();
protected:
	QVector<QWidget*> m_widgetList;
};

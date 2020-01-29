/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include <QMap>
#include <QSharedPointer>
#include <QVector>

class iAAttributeDescriptor;
class MainWindow;
class MdiChild;

class QWidget;
class QString;
class QStringList;

//! Dialog asking the user for some given parameters.
class open_iA_Core_API iAParameterDlg : public QDialog, public Ui_CommonInput
{
	Q_OBJECT
public:
	//! Create dialog with the given parameters.
	//! @param parent the parent widget
	//! @param title  the dialog title
	//! @param parmaeters list of parameters (name, type, value, range, ...)
	//! @param descr an optional description text, displayed on top of the dialog
	iAParameterDlg( QWidget *parent, QString const & title, QVector<QSharedPointer<iAAttributeDescriptor> > parameters, QString const & descr = QString());
	QMap<QString, QVariant> parameterValues() const;
	void showROI();
	int exec() override;
	void setSourceMdi(MdiChild* child, MainWindow* mainWnd);
	QVector<QWidget*> widgetList();

private slots:
	void updatedROI(QString text);
	void sourceChildClosed();
	void selectFilter();

private:
	QWidget * m_container;
	int m_roi[6];
	QVector<int> m_filterWithParameters;
	MdiChild * m_sourceMdiChild;
	MainWindow * m_mainWnd;
	bool m_sourceMdiChildClosed;
	QVector<QWidget*> m_widgetList;
	QVector<QSharedPointer<iAAttributeDescriptor> > m_parameters;

	void updateROIPart(QString const& partName, QString const& value);
};

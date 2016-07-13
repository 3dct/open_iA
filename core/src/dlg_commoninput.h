/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"
#include "ui_CommonInput.h"

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>
#include <QTextDocument>

class MdiChild;
class QWidget;
class QErrorMessage;
class QLabel;
class QScrollArea;

class open_iA_Core_API dlg_commoninput : public QDialog, public Ui_CommonInput
{
	Q_OBJECT

public:
	//class constructor
	dlg_commoninput ( QWidget *parent, QString winTitel, int n, QStringList inList, QList<QVariant> inPara, QTextDocument *fDescr = new QTextDocument( 0 ), bool modal = true);
	
	void setComboValues ( QList<QVariant> inCombo ){inComboValue = inCombo;}; 

	QStringList getWidgetList();
	
	QList<double> getValues();	
	QList<int> getCheckValues();	
	QStringList getComboBoxValues();
	QList<int> getComboBoxIndices();
	QStringList getText();
	QList<double> getSpinBoxValues();
	QList<double> getDoubleSpinBoxValues();

	double getParameterValue(QString name);	
	void updateValues(QList<QVariant>);

	void connectMdiChild(MdiChild *child);
	
private:
	
	int numPara;
	double outValue;
	QList<double> outValueList;
	QList<int> outCheckList;
	QStringList outComboValues, outTextList;
	QList<int> outComboIndices;
	QList<QLabel*> listLabel;
	QErrorMessage *eMessage;
	QList<QVariant> inComboValue;
	int NoofComboBox;
	QString tStr;
	QScrollArea *scrollArea;
	QWidget *container; 
	QGridLayout *containerLayout;
	int selectedComboBoxPos; 
	
protected:
	QStringList widgetList;

};

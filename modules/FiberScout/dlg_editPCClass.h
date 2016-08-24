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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#ifndef DLG_EDITPCCLASS_H
#define DLG_EDITPCCLASS_H

#include <QDialog>

class QString;
class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QLabel;
class QMessageBox;

class dlg_editPCClass : public QDialog
{
	Q_OBJECT

public:
	dlg_editPCClass(QWidget *parent = 0);
	~dlg_editPCClass();

	static QString getClassInfo(QWidget *parent, const QString &title, const QString &text, QColor *color, bool *ok);
	void setTextValue(const QString &text);
	QString getTextValue();
	void setColor(QColor *color);
	void getColor(QColor *color);
	void setupConnections();

signals:

private slots:
	void getColorDialog();
	void notifyTextChanged();

private:
	QLabel *cNameLabel;
	QLineEdit *nameEdit;
	QPushButton *colorButton;
	QLabel *cColorLabel;
	QDialogButtonBox *buttonBox;
	QColor dcolor;
};

#endif
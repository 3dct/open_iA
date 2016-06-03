/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef DLG_FILEEDIT_H
#define DLG_FILEEDIT_H
// Ui
#include "ui_iA4DCTFileEditDialog.h"
// Qt
#include <QDialog>
#include <QString>

class dlg_fileEdit : public QDialog, public Ui::FileEditDialog
{
	Q_OBJECT
public:
				dlg_fileEdit(QWidget* parent = 0);
				~dlg_fileEdit();

	QString		getFilePath();
	QString		getFileName();


protected slots:
	void		onBrowseButtonClick();
	void		fileTypeChanged(int index);
};

#endif // DLG_FILEEDIT_H
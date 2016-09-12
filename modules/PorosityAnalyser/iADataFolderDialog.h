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
 
#ifndef iADataFolderDialog_h__
#define iADataFolderDialog_h__

#include "ui_dataFolderDialog.h"

class iADataFolderDialog : public QDialog, public Ui_dataFolderDialog
{
	Q_OBJECT

public:
	iADataFolderDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~iADataFolderDialog();
	QString ResultsFolderName();
	QString DatasetsFolderName();

protected slots:
	void browseDataFolder();
	void browseDatasetsFolder();
	
private:
};
#endif // iADataFolderDialog_h__

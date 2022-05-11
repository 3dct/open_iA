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

// Ui
#include "ui_iA4DCTFileOpenDialog.h"
// iA
#include "iA4DCTData.h"
#include "iA4DCTFileData.h"
// Qt
#include <QDialog>
#include <QStandardItemModel>

// This class provides a dialog for opening 4DCT files. The dialog
// gives of available files for current 4dct project and it returns
// the path of the selected file.
class dlg_4DCTFileOpen : public QDialog, public Ui::FileOpenDialog
{
	Q_OBJECT
public:
						dlg_4DCTFileOpen( QWidget * parent );
	void				setData( iA4DCTData * data );
	iA4DCTFileData		getFile( );

public slots:
	void				accept( );
	void				onTreeViewDoubleClicked( const QModelIndex & index );

private:
	void				setFileAndClose( const QModelIndex & index );
	iA4DCTData *		m_data;
	QStandardItemModel	m_model;
	iA4DCTFileData		m_file;
};

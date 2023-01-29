// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

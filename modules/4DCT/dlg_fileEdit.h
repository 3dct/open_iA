// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTFileEditDialog.h"
// Qt
#include <QDialog>
#include <QString>

class dlg_fileEdit : public QDialog, public Ui::FileEditDialog
{
	Q_OBJECT
public:
				dlg_fileEdit( QWidget* parent );
				~dlg_fileEdit( );

	QString		getFilePath( );
	QString		getFileName( );


protected slots:
	void		onBrowseButtonClick( );
	void		fileTypeChanged( int index );
};

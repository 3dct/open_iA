// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTDensityMapDialog.h"
// Qt
#include <QDialog>

class iA4DCTVisWin;

class dlg_2dDensityMap : public QDialog, public Ui::DensityMapDialog
{
	Q_OBJECT
public:
				dlg_2dDensityMap( QWidget * parent = 0 );
				~dlg_2dDensityMap( );
	void		setVisWin( iA4DCTVisWin * visWin );


private slots:
	void		defectButtonClicked( );
	void		labeledImgButtonClicked( );

private:
	iA4DCTVisWin *	m_visWin;
};

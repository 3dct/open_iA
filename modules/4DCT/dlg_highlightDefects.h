// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTHightlightDefectsDialog.h"
// Qt
#include <QDialog>

class iA4DCTVisWin;

class dlg_highlightDefects : public QDialog, public Ui::HighlightDefectsDialog
{
	Q_OBJECT

public:
				dlg_highlightDefects( QWidget * parent );
				~dlg_highlightDefects( );
	void		setVisWin( iA4DCTVisWin * visWin );


private slots:
	void		pulloutsButtonClicked( );
	void		breakagesButtonClicked( );
	void		debondingsButtonClicked( );
	void		cracksButtonClicked( );
	void		labeledImgButtonClicked( );


private:
	iA4DCTVisWin *	m_visWin;
};

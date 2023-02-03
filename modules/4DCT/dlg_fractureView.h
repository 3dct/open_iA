// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTFractureViewDialog.h"
// iA
#include "iA4DCTData.h"
// Qt
#include <QDialog>


class dlg_fractureView : public QDialog, private Ui::FractureViewDialog
{
	Q_OBJECT
public:
						dlg_fractureView( QWidget* patent = 0 );
						dlg_fractureView( iA4DCTData * data, QWidget* parent = 0 );
						~dlg_fractureView( );

	int					getStageIndex( );
	int					getImageIndex( );

private:
	// internal data
	iA4DCTData *		m_data;

private slots:
	void				stageCurrentIndexChanged( int ind );
};

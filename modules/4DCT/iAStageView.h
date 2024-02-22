// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iA4DCTStageData.h"
// Ui
#include "ui_iA4DCTStageView.h"

class iAStageView : public QWidget, public Ui::StageView
{
	Q_OBJECT

public:
						iAStageView( QWidget * parent = 0 );
						~iAStageView( );
	void				setData( iA4DCTStageData * data );
	iA4DCTStageData *	getData( );
	void				updateWidgets( );
	void				addFile( );

private:
	iA4DCTStageData *	m_data;

	private slots:
	void				forceValueChanged( int val );
};

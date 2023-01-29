// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTRegionViewDialog.h"
// Qt
#include <QDialog>
#include <QString>
// iA
#include "iA4DCTFileData.h"
#include "iA4DCTData.h"

class dlg_regionView : public QDialog, public Ui::RegionViewDialog
{
	Q_OBJECT
public:
						dlg_regionView( QWidget * parent );
	void				setData( iA4DCTData * data );
	QString				getImagePath( );
	QString				getImageName( );
	double				getThreshold( );


private slots:
	void				onSelectButtonClicked( );


private:
	iA4DCTData *		m_data;
	iA4DCTFileData		m_file;
};

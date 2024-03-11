// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_2dDensityMap.h"

#include "iA4DCTVisWin.h"
#include "iA4DCTFileData.h"

dlg_2dDensityMap::dlg_2dDensityMap( QWidget * parent /*= 0 */ )
	: QDialog( parent )
{
	setupUi(this);
	connect(pbDefect,     &QPushButton::clicked, this, &dlg_2dDensityMap::defectButtonClicked);
	connect(pbLabeledImg, &QPushButton::clicked, this, &dlg_2dDensityMap::labeledImgButtonClicked);
	// default parameters
	cbDefect->setColor( QColor( 255, 0, 0 ) );
}

dlg_2dDensityMap::~dlg_2dDensityMap( )
{ /* not implemented */ }

void dlg_2dDensityMap::setVisWin( iA4DCTVisWin * visWin )
{
	m_visWin = visWin;
}

void dlg_2dDensityMap::defectButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leDefect->setText( fileData.Path );
}

void dlg_2dDensityMap::labeledImgButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leLabeledImg->setText( fileData.Path );
}

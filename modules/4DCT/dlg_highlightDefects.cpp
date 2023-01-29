// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_highlightDefects.h"

#include "iA4DCTVisWin.h"
#include "iA4DCTFileData.h"

const QColor CrackColor( 91, 155, 213 );		// blue
const QColor DebondingColor( 0, 176, 80 );		// green
const QColor PulloutColor( 255, 192, 0 );		// yellow
const QColor BreakageColor( 255, 0, 0 );		// red

dlg_highlightDefects::dlg_highlightDefects( QWidget * parent )
	: QDialog( parent )
{
	setupUi( this );
	connect( pbPullouts,   &QPushButton::clicked, this, &dlg_highlightDefects::pulloutsButtonClicked);
	connect( pbBreakages,  &QPushButton::clicked, this, &dlg_highlightDefects::breakagesButtonClicked);
	connect( pbCracks,     &QPushButton::clicked, this, &dlg_highlightDefects::cracksButtonClicked);
	connect( pbDebondings, &QPushButton::clicked, this, &dlg_highlightDefects::debondingsButtonClicked);
	connect( pbLabeledImg, &QPushButton::clicked, this, &dlg_highlightDefects::labeledImgButtonClicked);

	cbPullouts->setColor( PulloutColor );
	cbBreakages->setColor( BreakageColor );
	cbCracks->setColor( CrackColor );
	cbDebondings->setColor( DebondingColor );
}

dlg_highlightDefects::~dlg_highlightDefects( )
{ /* not implemented */
}

void dlg_highlightDefects::setVisWin( iA4DCTVisWin * visWin )
{
	m_visWin = visWin;
}

void dlg_highlightDefects::pulloutsButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	lePullouts->setText( fileData.Path );
}

void dlg_highlightDefects::breakagesButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leBreakages->setText( fileData.Path );
}

void dlg_highlightDefects::debondingsButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leDebondings->setText( fileData.Path );
}

void dlg_highlightDefects::cracksButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leCracks->setText( fileData.Path );
}

void dlg_highlightDefects::labeledImgButtonClicked( )
{
	iA4DCTFileData fileData;
	m_visWin->showDialog( fileData );
	leLabeledImg->setText( fileData.Path );
}

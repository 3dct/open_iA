/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
	connect( pbPullouts, SIGNAL( clicked( ) ), this, SLOT( pulloutsButtonClicked( ) ) );
	connect( pbBreakages, SIGNAL( clicked( ) ), this, SLOT( breakagesButtonClicked( ) ) );
	connect( pbCracks, SIGNAL( clicked( ) ), this, SLOT( cracksButtonClicked( ) ) );
	connect( pbDebondings, SIGNAL( clicked( ) ), this, SLOT( debondingsButtonClicked( ) ) );
	connect( pbLabeledImg, SIGNAL( clicked( ) ), this, SLOT( labeledImgButtonClicked( ) ) );

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

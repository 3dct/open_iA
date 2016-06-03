/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_2dDensityMap.h"
// iA
#include "iA4DCTVisWin.h"


dlg_2dDensityMap::dlg_2dDensityMap( QWidget * parent /*= 0 */ )
	: QDialog( parent )
{
	setupUi( this );
	connect( pbDefect,		SIGNAL( clicked( ) ), this, SLOT( defectButtonClicked( ) ) );
	connect( pbLabeledImg,	SIGNAL( clicked( ) ), this, SLOT( labeledImgButtonClicked( ) ) );
	// default parameters
	cbDefect->setColor( QColor( 255, 0, 0 ) );
}

dlg_2dDensityMap::~dlg_2dDensityMap()
{ /* not implemented */ }

void dlg_2dDensityMap::setVisWin( iA4DCTVisWin * visWin )
{
	m_visWin = visWin;
}

void dlg_2dDensityMap::defectButtonClicked()
{
	QString file;
	m_visWin->showDialog( file );
	leDefect->setText( file );
}

void dlg_2dDensityMap::labeledImgButtonClicked()
{
	QString file;
	m_visWin->showDialog( file );
	leLabeledImg->setText( file );
}

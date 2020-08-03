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

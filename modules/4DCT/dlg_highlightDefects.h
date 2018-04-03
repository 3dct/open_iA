/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#pragma once

// Ui
#include "ui_iA4DCTHeightlightDefectsDialog.h"
// Qt
#include <QDialog>

class iA4DCTVisWin;

class dlg_highlightDefects : public QDialog, public Ui::HeighlightDefectsDialog
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

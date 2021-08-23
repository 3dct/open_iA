/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "dlg_regionView.h"

#include "dlg_4DCTFileOpen.h"

dlg_regionView::dlg_regionView( QWidget * parent )
	: QDialog( parent )
{
	setupUi( this );
	connect( pbSelectImage, &QPushButton::clicked, this, &dlg_regionView::onSelectButtonClicked);
}

void dlg_regionView::onSelectButtonClicked( )
{
	dlg_4DCTFileOpen dialog( this );
	dialog.setData( m_data );
	if (dialog.exec() != QDialog::Accepted)
	{
		return;
	}
	m_file = dialog.getFile( );
	lFilename->setText( m_file.Name );
}

void dlg_regionView::setData( iA4DCTData * newData )
{
	m_data = newData;
}

QString dlg_regionView::getImagePath( )
{
	return m_file.Path;
}

QString dlg_regionView::getImageName( )
{
	return m_file.Name;
}

double dlg_regionView::getThreshold( )
{
	return dspThreshold->value( );
}

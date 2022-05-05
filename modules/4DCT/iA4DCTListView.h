/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

// iA
#include "iA4DCTFileData.h"
// Qt
#include <QListView>

class QMenu;
class iAStageView;

class iA4DCTListView : public QListView
{
	Q_OBJECT

public:
				iA4DCTListView( QWidget* parent = 0 );
				~iA4DCTListView( );
	void		setStageView( iAStageView* stageView );
	void		updateData( );

protected:
	void		contextMenuEvent( QContextMenuEvent* event );

private slots:
	void		openFile( );
	void		addFile( );

private:
	QMenu *					m_menu;
	QAction *				m_actOpen;
	QAction *				m_actNew;
	QList<iA4DCTFileData> *	m_data;
	iAStageView *			m_stageView;
};

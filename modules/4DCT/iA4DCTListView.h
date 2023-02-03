// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

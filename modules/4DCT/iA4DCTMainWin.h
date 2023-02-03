// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iA4DCTData.h"
#include "ui_iA4DCTMainWin.h"
// Qt
#include <QMainWindow>
#include <QString>
#include <QVector>

class iAMainWindow;
class iAStageView;

class iA4DCTMainWin : public QMainWindow, public Ui::iA4DMainWin
{
	Q_OBJECT

public:
								iA4DCTMainWin(iAMainWindow* parent = nullptr);
								~iA4DCTMainWin( );
	void						load( QString path );
	void						save( QString path );
	iA4DCTData *				getStageData( );
	iAStageView *				addStage( iA4DCTStageData stageData );
	double *					getSize( );
	void						setSize( double * size );

public slots:
	void						save( );
	void						openVisualizationWin( );
	void						addButtonClick( );

private:
	iAMainWindow *				m_mainWnd;
	QVector<iAStageView *>		m_stages;
	double						m_size[3];
	iA4DCTData					m_data;
};

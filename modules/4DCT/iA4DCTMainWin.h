/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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

// iA
#include "iA4DCTData.h"
#include "qthelper/iAQTtoUIConnector.h"
#include "ui_iA4DCTMainWin.h"
// Qt
#include <QMainWindow>
#include <QString>
#include <QVector>

class MainWindow;
class iAStageView;

class iA4DCTMainWin : public QMainWindow, public Ui::iA4DMainWin
{
	Q_OBJECT

public:
								iA4DCTMainWin( MainWindow* parent = 0 );
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
	MainWindow *				m_mainWnd;
	QVector<iAStageView *>		m_stages;
	double						m_size[3];
	iA4DCTData					m_data;
};

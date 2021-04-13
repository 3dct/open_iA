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
#pragma once

// Ui
#include "ui_iA4DCTPlaneDockWidget.h"
// Qt
#include <QDockWidget>

class iAPlaneVisModule;
class iA4DCTVisWin;

class iA4DCTPlaneDockWidget : public QDockWidget, public Ui::PlaneDockWidget
{
	Q_OBJECT
public:
				iA4DCTPlaneDockWidget( iA4DCTVisWin * parent );
	void		attachTo( iAPlaneVisModule * module );

signals:
	void		updateRenderWindow( );

private slots:
	void		changedSlice( int val );
	void		changedOpacity( int val );
	void		enableShading( int state );
	void		setXYDir( );
	void		setXZDir( );
	void		setYZDir( );
	void		hightlightDefectsButtonClicked( );
	void		densityMapButtonClicked( );
	void		nextSlice( );
	void		previousSlice( );
	void		enableHighlighting( int state );

private:
	void		rescaleSliceSlider( int max, int val );

	iAPlaneVisModule *		m_visModule;
	iA4DCTVisWin *			m_visWin;
};
// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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

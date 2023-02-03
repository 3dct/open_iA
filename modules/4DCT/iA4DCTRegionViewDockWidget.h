// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTRegionViewDockWidget.h"
// Qt
#include <QDockWidget>

class iARegionVisModule;

class iA4DCTRegionViewDockWidget : public QDockWidget, public Ui::RegionViewDockWidget
{
	Q_OBJECT
public:
				iA4DCTRegionViewDockWidget( QWidget * parent );
	void		attachTo( iARegionVisModule * visModule );

signals:
	void		updateRenderWindow( );

private:
	iARegionVisModule *		m_visModule;

private slots:
	void		onSilhoetteWidthChanged( int val );
	void		onSilhoetteOpacityChanged( int val );
	void		onSurfaceOpacityChanged( int val );
	void		onSilhoetteColorChanged( const QColor & col );
	void		onSurfaceColorChanged( const QColor & col );
};

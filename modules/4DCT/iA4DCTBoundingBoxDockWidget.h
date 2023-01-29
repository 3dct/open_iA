// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTBoundingBoxDockWidget.h"
// Qt
#include <QDockWidget>

class iABoundingBoxVisModule;
class vtkRenderer;

class iA4DCTBoundingBoxDockWidget : public QDockWidget, public Ui::BoundingBoxDockWidget
{
	Q_OBJECT
public:
				iA4DCTBoundingBoxDockWidget( QWidget * parent );
	void		attachTo( iABoundingBoxVisModule * boundingBox );
	void		setRenderer( vtkRenderer * renderer );

signals:
	void		updateRenderWindow( );

private slots:
	void		changeColor( const QColor & col );
	void		changeLineWidth( int val );

private:
	iABoundingBoxVisModule *	m_boundingBox;
	vtkRenderer *				m_renderer;
};

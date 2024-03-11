// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTDefectVisDockWidget.h"
// Qt
#include <QDockWidget>

class iADefectVisModule;
class vtkRenderer;

class iA4DCTDefectVisDockWidget : public QDockWidget, public Ui::DefectVisDockWidget
{
	Q_OBJECT
public:
				iA4DCTDefectVisDockWidget( QWidget * parent );
	void		attachTo( iADefectVisModule * defectVis );
	void		setRenderer( vtkRenderer * renderer );

signals:
	void		updateRenderWindow( );

private slots:
	void		changeColor( const QColor & col );
	void		changeOpacity( int val );

private:
	iADefectVisModule *		m_defectVis;
	vtkRenderer *			m_renderer;
};

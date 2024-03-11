// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Ui
#include "ui_iA4DCTFractureVisDockWidget.h"
// iA
#include "iA4DCTData.h"
// QT
#include <QDockWidget>

class iAFractureVisModule;

class iA4DCTFractureVisDockWidget : public QDockWidget, public Ui::FractureVisDockWidget
{
	Q_OBJECT
public:
				iA4DCTFractureVisDockWidget( QWidget * parent );
	void		attachTo( iAFractureVisModule * visModule );
	void		setData( iA4DCTData * data );

private slots:
	void		onSaveButtonClicked( );
	void		onColorizeButtonClicked( );
	void		onLowIntensityColorChanged( const QColor & color );
	void		onHighIntensityColorChanged( const QColor & color );
	void		onColorChanged( const QColor & color );
	void		onAmbientValueChanged( int val );
	void		onOpacityValueChanged( int val );

signals:
	void		updateRenderWindow( );

private:
	iAFractureVisModule *		m_visModule;
	iA4DCTData *				m_data;
};

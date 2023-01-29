// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTFractureVisDockWidget.h"

#include "iAFractureVisModule.h"
#include "dlg_4DCTFileOpen.h"
#include "iA4DCTSettings.h"

#include <QSettings>
#include <QFileDialog>
#include <QString>

iA4DCTFractureVisDockWidget::iA4DCTFractureVisDockWidget( QWidget * parent )
	: QDockWidget( parent )
	, m_visModule( nullptr )
	, m_data( nullptr )
{
	setupUi( this );
	connect( pbSave, &QPushButton::clicked, this, &iA4DCTFractureVisDockWidget::onSaveButtonClicked);
	connect( pbColorize, &QPushButton::clicked, this, &iA4DCTFractureVisDockWidget::onColorizeButtonClicked);
	connect( cbLowIntensity, &iAColorBox::colorChanged, this, &iA4DCTFractureVisDockWidget::onLowIntensityColorChanged);
	connect( cbHighIntensity, &iAColorBox::colorChanged, this, &iA4DCTFractureVisDockWidget::onHighIntensityColorChanged);
	connect( cbColor, &iAColorBox::colorChanged, this, &iA4DCTFractureVisDockWidget::onColorChanged);
	connect( sAmbient, &QSlider::valueChanged, this, &iA4DCTFractureVisDockWidget::onAmbientValueChanged);
	connect( sOpacity, &QSlider::valueChanged, this, &iA4DCTFractureVisDockWidget::onOpacityValueChanged);
}

void iA4DCTFractureVisDockWidget::attachTo( iAFractureVisModule * visModule )
{
	m_visModule = visModule;
}

void iA4DCTFractureVisDockWidget::onSaveButtonClicked( )
{
	if( !m_visModule )
		return;

	QSettings settings;
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Save surface" ),
													 settings.value( S_4DCT_SAVE_SURFACE_DIR ).toString( ), tr( "Images (*.mhd)" ) );
	if( fileName.isEmpty( ) ) {
		return;
	}
	settings.setValue( S_4DCT_SAVE_SURFACE_DIR, fileName );
	m_visModule->save( fileName );
}

void iA4DCTFractureVisDockWidget::onColorizeButtonClicked( )
{
	if( !m_visModule )
		return;

	dlg_4DCTFileOpen fileOpen( this );
	fileOpen.setData( m_data );
	if( fileOpen.exec( ) != QDialog::Accepted )
		return;

	m_visModule->setColorMap( fileOpen.getFile( ).Path );
	emit updateRenderWindow( );
}

void iA4DCTFractureVisDockWidget::setData( iA4DCTData * newData )
{
	m_data = newData;
}

void iA4DCTFractureVisDockWidget::onLowIntensityColorChanged( const QColor & color )
{
	m_visModule->setLowIntensityColor( color );
	emit updateRenderWindow( );
}

void iA4DCTFractureVisDockWidget::onHighIntensityColorChanged( const QColor & color )
{
	m_visModule->setHighIntensityColor( color );
	emit updateRenderWindow( );
}

void iA4DCTFractureVisDockWidget::onAmbientValueChanged( int val )
{
	m_visModule->setAmbient( (double)val / sAmbient->maximum( ) );
	emit updateRenderWindow( );
}

void iA4DCTFractureVisDockWidget::onColorChanged( const QColor & color )
{
	m_visModule->setColor( color );
	emit updateRenderWindow( );
}

void iA4DCTFractureVisDockWidget::onOpacityValueChanged( int val )
{
	m_visModule->setOpacity( (double)val / sOpacity->maximum( ) );
	emit updateRenderWindow( );
}

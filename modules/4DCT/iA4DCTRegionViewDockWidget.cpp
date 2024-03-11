// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTRegionViewDockWidget.h"

#include "iARegionVisModule.h"

iA4DCTRegionViewDockWidget::iA4DCTRegionViewDockWidget( QWidget * parent )
	: QDockWidget( parent )
	, m_visModule( nullptr )
{
	setupUi( this );
	connect( sSilhoetteWidth, &QSlider::valueChanged, this, &iA4DCTRegionViewDockWidget::onSilhoetteWidthChanged);
	connect( sSilhoetteOpacity, &QSlider::valueChanged, this, &iA4DCTRegionViewDockWidget::onSilhoetteOpacityChanged);
	connect( sSurfaceOpacity, &QSlider::valueChanged, this, &iA4DCTRegionViewDockWidget::onSurfaceOpacityChanged);
	connect( cbSilhoetteColor, &iAColorBox::colorChanged, this, &iA4DCTRegionViewDockWidget::onSilhoetteColorChanged);
	connect( cbSurfaceColor, &iAColorBox::colorChanged, this, &iA4DCTRegionViewDockWidget::onSurfaceColorChanged);
}

void iA4DCTRegionViewDockWidget::attachTo( iARegionVisModule * visModule )
{
	m_visModule = visModule;
	sSilhoetteWidth->setValue( m_visModule->settings.SilhoetteWidth );
	sSilhoetteOpacity->setValue( m_visModule->settings.SilhoetteOpacity * sSilhoetteOpacity->maximum( ) );
	sSurfaceOpacity->setValue( m_visModule->settings.SurfaceOpacity   * sSurfaceOpacity->maximum( ) );
	cbSilhoetteColor->setColor( m_visModule->settings.SilhoetteColor );
	cbSurfaceColor->setColor( m_visModule->settings.SurfaceColor );
}

void iA4DCTRegionViewDockWidget::onSilhoetteWidthChanged( int val )
{
	if( m_visModule == nullptr )
		return;
	double width = 0.5 + 7.5 * (double)val / sSilhoetteWidth->maximum( );
	m_visModule->setSilhoetteLineWidth( width );
	emit updateRenderWindow( );
}

void iA4DCTRegionViewDockWidget::onSilhoetteOpacityChanged( int val )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setSilhoetteOpacity( (double)val / sSilhoetteOpacity->maximum( ) );
	emit updateRenderWindow( );
}

void iA4DCTRegionViewDockWidget::onSurfaceOpacityChanged( int val )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setSurfaceOpacity( (double)val / sSurfaceOpacity->maximum( ) );
	emit updateRenderWindow( );
}

void iA4DCTRegionViewDockWidget::onSilhoetteColorChanged( const QColor & col )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setSilhoetteColor( col.redF( ), col.greenF( ), col.blueF( ) );
	emit updateRenderWindow( );
}

void iA4DCTRegionViewDockWidget::onSurfaceColorChanged( const QColor & col )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setSurfaceColor( col.redF( ), col.greenF( ), col.blueF( ) );
	emit updateRenderWindow( );
}

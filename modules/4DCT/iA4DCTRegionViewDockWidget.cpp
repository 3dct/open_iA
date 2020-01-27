/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iA4DCTRegionViewDockWidget.h"

#include "iARegionVisModule.h"

iA4DCTRegionViewDockWidget::iA4DCTRegionViewDockWidget( QWidget * parent )
	: QDockWidget( parent )
	, m_visModule( nullptr )
{
	setupUi( this );
	connect( sSilhoetteWidth, SIGNAL( valueChanged( int ) ), this, SLOT( onSilhoetteWidthChanged( int ) ) );
	connect( sSilhoetteOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( onSilhoetteOpacityChanged( int ) ) );
	connect( sSurfaceOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( onSurfaceOpacityChanged( int ) ) );
	connect( cbSilhoetteColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( onSilhoetteColorChanged( QColor ) ) );
	connect( cbSurfaceColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( onSurfaceColorChanged( QColor ) ) );
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

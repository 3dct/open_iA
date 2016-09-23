/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "pch.h"
#include "iA4DCTPlaneDockWidget.h"
// iA
#include "iAPlaneVisModule.h"
#include "iA4DCTVisWin.h"
#include "dlg_highlightDefects.h"
#include "dlg_2dDensityMap.h"
// Qt
#include <QString>

iA4DCTPlaneDockWidget::iA4DCTPlaneDockWidget( iA4DCTVisWin * parent )
	: QDockWidget( parent )
	, m_visModule( nullptr )
	, m_visWin( parent )
{
	setupUi( this );

	connect( sSlice, SIGNAL( valueChanged( int ) ), this, SLOT( changedSlice( int ) ) );
	connect( sOpacity, SIGNAL( valueChanged( int ) ), this, SLOT( changedOpacity( int ) ) );
	connect( cbShading, SIGNAL( stateChanged( int ) ), this, SLOT( enableShading( int ) ) );
	connect( rbXY, SIGNAL( clicked( ) ), this, SLOT( setXYDir( ) ) );
	connect( rbXZ, SIGNAL( clicked( ) ), this, SLOT( setXZDir( ) ) );
	connect( rbYZ, SIGNAL( clicked( ) ), this, SLOT( setYZDir( ) ) );
	connect( pbHighlightDefects, SIGNAL( clicked( ) ), this, SLOT( hightlightDefectsButtonClicked( ) ) );
	connect( pbDensityMap, SIGNAL( clicked( ) ), this, SLOT( densityMapButtonClicked( ) ) );
}

void iA4DCTPlaneDockWidget::attachTo( iAPlaneVisModule * module )
{
	m_visModule = module;
	sSlice->setValue( m_visModule->settings.Slice * sSlice->maximum( ) );
	sOpacity->setValue( m_visModule->settings.Opacity * sOpacity->maximum( ) );
	cbShading->setChecked( m_visModule->settings.Shading );
	switch( m_visModule->settings.Dir )
	{
	case iAPlaneVisSettings::Direction::XY:
		rbXY->setChecked( true );
		break;
	case iAPlaneVisSettings::Direction::XZ:
		rbXZ->setChecked( true );
		break;
	case iAPlaneVisSettings::Direction::YZ:
		rbYZ->setChecked( true );
		break;
	}
}

void iA4DCTPlaneDockWidget::changedSlice( int val )
{
	if( m_visModule == nullptr )
		return;
	double doubleVal = (double)val / sSlice->maximum( );
	m_visModule->setSlice( doubleVal );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::changedOpacity( int val )
{
	if( m_visModule == nullptr )
		return;
	double doubleVal = (double)val / sOpacity->maximum( );
	m_visModule->setOpacity( doubleVal );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::enableShading( int state )
{
	if( m_visModule == nullptr )
		return;
	if( state == Qt::Checked ) {
		m_visModule->enableShading( );
	} else {
		m_visModule->disableShading( );
	}
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setXYDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirXY( );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setXZDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirXZ( );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setYZDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirYZ( );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::hightlightDefectsButtonClicked( )
{
	dlg_highlightDefects dialog( this );
	dialog.setVisWin( m_visWin );
	if( dialog.exec( ) != QDialog::Accepted )
		return;

	QVector<QString> defectsLists; QVector<QColor> defectsColors;
	QString pullouts = dialog.lePullouts->text( );
	QString breakages = dialog.leBreakages->text( );
	QString debondings = dialog.leDebondings->text( );
	QString cracks = dialog.leCracks->text( );

	if( !debondings.isEmpty( ) )
	{
		defectsLists.push_back( dialog.leDebondings->text( ) );
		defectsColors.push_back( dialog.cbDebondings->getColor( ) );
	}
	if( !breakages.isEmpty( ) )
	{
		defectsLists.push_back( dialog.leBreakages->text( ) );
		defectsColors.push_back( dialog.cbBreakages->getColor( ) );
	}
	if( !pullouts.isEmpty( ) )
	{
		defectsLists.push_back( dialog.lePullouts->text( ) );
		defectsColors.push_back( dialog.cbPullouts->getColor( ) );
	}
	if( !cracks.isEmpty( ) )
	{
		defectsLists.push_back( dialog.leCracks->text( ) );
		defectsColors.push_back( dialog.cbCracks->getColor( ) );
	}


	m_visModule->highlightDefects<unsigned short>( defectsLists, defectsColors, dialog.leLabeledImg->text( ) );
}

void iA4DCTPlaneDockWidget::densityMapButtonClicked( )
{
	dlg_2dDensityMap dialog( this );
	dialog.setVisWin( m_visWin );
	if( dialog.exec( ) != QDialog::Accepted )
		return;

	int size[3] = { dialog.sbSizeX->value( ), dialog.sbSizeY->value( ), dialog.sbSizeZ->value( ) };
	m_visModule->densityMap<unsigned short>( dialog.leDefect->text( ),
											 dialog.cbDefect->getColor( ),
											 dialog.leLabeledImg->text( ),
											 size );
}
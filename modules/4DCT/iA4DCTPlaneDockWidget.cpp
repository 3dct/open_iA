// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTPlaneDockWidget.h"

#include "iAPlaneVisModule.h"
#include "iA4DCTVisWin.h"
#include "dlg_highlightDefects.h"
#include "dlg_2dDensityMap.h"

#include <QString>

iA4DCTPlaneDockWidget::iA4DCTPlaneDockWidget( iA4DCTVisWin * parent )
	: QDockWidget( parent )
	, m_visModule( nullptr )
	, m_visWin( parent )
{
	setupUi( this );

	connect( sSlice, &QSlider::valueChanged, this, &iA4DCTPlaneDockWidget::changedSlice);
	connect( sOpacity, &QSlider::valueChanged, this, &iA4DCTPlaneDockWidget::changedOpacity );
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
	connect(cbShading, &QCheckBox::stateChanged, this, &iA4DCTPlaneDockWidget::enableShading);
	connect(cbHighlighting, &QCheckBox::stateChanged, this, &iA4DCTPlaneDockWidget::enableHighlighting);
#else
	connect(cbShading, &QCheckBox::checkStateChanged, this, &iA4DCTPlaneDockWidget::enableShading);
	connect(cbHighlighting, &QCheckBox::checkStateChanged, this, &iA4DCTPlaneDockWidget::enableHighlighting);
#endif
	connect( rbXY, &QRadioButton::clicked, this, &iA4DCTPlaneDockWidget::setXYDir);
	connect( rbXZ, &QRadioButton::clicked, this, &iA4DCTPlaneDockWidget::setXZDir);
	connect( rbYZ, &QRadioButton::clicked, this, &iA4DCTPlaneDockWidget::setYZDir);
	connect( pbHighlightDefects, &QPushButton::clicked, this, &iA4DCTPlaneDockWidget::hightlightDefectsButtonClicked);
	connect( pbDensityMap, &QPushButton::clicked, this, &iA4DCTPlaneDockWidget::densityMapButtonClicked);
	connect( pbNext, &QPushButton::clicked, this, &iA4DCTPlaneDockWidget::nextSlice);
	connect( pbPrevious, &QPushButton::clicked, this, &iA4DCTPlaneDockWidget::previousSlice);
}

void iA4DCTPlaneDockWidget::attachTo( iAPlaneVisModule * module )
{
	m_visModule = module;
	sOpacity->setValue( m_visModule->settings.Opacity * sOpacity->maximum( ) );
	cbShading->setChecked( m_visModule->settings.Shading );
	int size[3]; m_visModule->getImageSize( size );
	switch( m_visModule->settings.Dir )
	{
	case iAPlaneVisSettings::Direction::XY:
	{
		sSlice->setMaximum( size[0] );
		sSlice->setValue( m_visModule->settings.Slice[0] );
		setXYDir( );
		break;
	}
	case iAPlaneVisSettings::Direction::XZ:
	{
		sSlice->setMaximum( size[1] );
		sSlice->setValue( m_visModule->settings.Slice[1] );
		setXZDir( );
		break;
	}
	case iAPlaneVisSettings::Direction::YZ:
	{
		sSlice->setMaximum( size[2] );
		sSlice->setValue( m_visModule->settings.Slice[2] );
		setYZDir( );
		break;
	}
	}
}

void iA4DCTPlaneDockWidget::changedSlice( int val )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setSlice( val );
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
	if( state == Qt::Checked )
		m_visModule->enableShading( );
	else
		m_visModule->disableShading( );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setXYDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirXY( );
	int size[3]; m_visModule->getImageSize( size );
	rescaleSliceSlider( size[2], m_visModule->settings.Slice[0] );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setXZDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirXZ( );
	int size[3]; m_visModule->getImageSize( size );
	rescaleSliceSlider( size[1], m_visModule->settings.Slice[1] );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::setYZDir( )
{
	if( m_visModule == nullptr )
		return;
	m_visModule->setDirYZ( );
	int size[3]; m_visModule->getImageSize( size );
	rescaleSliceSlider( size[0], m_visModule->settings.Slice[2] );
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
	cbHighlighting->setEnabled( true ); cbHighlighting->setChecked( true );
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

void iA4DCTPlaneDockWidget::nextSlice( )
{
	sSlice->setValue( sSlice->value( ) + 1 );
}

void iA4DCTPlaneDockWidget::previousSlice( )
{
	sSlice->setValue( sSlice->value( ) - 1 );
}

void iA4DCTPlaneDockWidget::enableHighlighting( int state )
{
	if( state == Qt::Checked )
		m_visModule->enableHighlighting( true );
	else
		m_visModule->enableHighlighting( false );
	emit updateRenderWindow( );
}

void iA4DCTPlaneDockWidget::rescaleSliceSlider( int max, int val )
{
	if( max > val )	{
		sSlice->setMaximum( max );
		sSlice->setValue( val );
	} else {
		sSlice->setValue( val );
		sSlice->setMaximum( max );
	}
}

// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTMainWin.h"

#include "iA4DCTData.h"
#include "iA4DCTDefects.h"
#include "iA4DCTProjectReaderWriter.h"
#include "iA4DCTSettings.h"
#include "iA4DCTVisWin.h"
#include "iAMhdFileInfo.h"
#include "iAPreviewMaker.h"
#include "iAStageView.h"

#include <iAMainWindow.h>

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QSettings>
#include <QStringList>
#include <QTextStream>

iA4DCTMainWin::iA4DCTMainWin( iAMainWindow* parent )
	: QMainWindow( parent )
	, m_mainWnd( parent )
{
	setupUi( this );

	m_size[0] = 100.; m_size[1] = 100.; m_size[2] = 100.;

	connect( actionAdd, &QAction::triggered, this, &iA4DCTMainWin::addButtonClick);
	connect( actionVisualization, &QAction::triggered, this, &iA4DCTMainWin::openVisualizationWin);
	connect( actionSave, &QAction::triggered, this, QOverload<>::of(&iA4DCTMainWin::save));
}

iA4DCTMainWin::~iA4DCTMainWin( )
{ }

void iA4DCTMainWin::load( QString settingsFile )
{
	iA4DCTProjectReaderWriter::load( this, settingsFile );
}

void iA4DCTMainWin::save( )
{
	QSettings settings;
	QString fileName = QFileDialog::getSaveFileName(
		m_mainWnd,
		tr( "Save 4DCT proj" ),
		settings.value( S_4DCT_SAVE_DIR ).toString( ),
		tr( "4DCT project (*.xml)" ) );

	QFileInfo file( fileName );
	if( !file.dir( ).exists( ) ) {
		return;
	}
	settings.setValue( S_4DCT_SAVE_DIR, file.absolutePath( ) );

	save( fileName );
}

void iA4DCTMainWin::save( QString settingsFile )
{
	iA4DCTProjectReaderWriter::save( this, settingsFile );
}

void iA4DCTMainWin::addButtonClick( )
{
	// open dialog
	QSettings settings;
	QString fileNamePath = settings.value( S_4DCT_ADD_BUTTON_DLG ).toString( );
	QString fileName = QFileDialog::getOpenFileName( m_mainWnd, "Open file", fileNamePath, "Meta header (*.mhd)" );
	if( !fileName.isEmpty( ) ) {
		settings.setValue( S_4DCT_ADD_BUTTON_DLG, fileName );
	}

	// does file exist
	QFileInfo fiMhd( fileName );
	if( !fiMhd.exists( ) )
		return;

	// add widget
	iA4DCTStageData stageData;
	stageData.Files.push_back( iA4DCTFileData( fiMhd.absoluteFilePath( ), fiMhd.baseName( ) ) );
	iAPreviewMaker::makeUsingType( fiMhd.absoluteFilePath( ), fiMhd.absolutePath( ) + "/thumbnail.png" );
	iA4DCTFileData file( fiMhd.absolutePath( ) + "/thumbnail.png", S_4DCT_THUMB_NAME );
	stageData.Files.push_back( file );
	addStage( stageData );

	if( m_stages.size( ) <= 1 )
	{
		iAMhdFileInfo mhdFileInfo( fileName );
		double dimSize[3]; double spacing[3];
		mhdFileInfo.getFileDimSize( dimSize );
		mhdFileInfo.getElementSpacing( spacing );
		double size[3] = { dimSize[0] * spacing[0],
						   dimSize[1] * spacing[1],
						   dimSize[2] * spacing[2] };
		setSize( size );
	}
}

void iA4DCTMainWin::openVisualizationWin( )
{
	iAMainWindow* mainWin = qobject_cast<iAMainWindow*>(QApplication::activeWindow());
	iA4DCTVisWin* visWin = new iA4DCTVisWin( this );
	visWin->setImageSize( m_size );
	visWin->setNumberOfStages( static_cast<int>(m_stages.size()) );
	mainWin->addSubWindow( visWin );
	visWin->showMaximized( );
}

iA4DCTData * iA4DCTMainWin::getStageData( )
{
	/*iA4DCTData data;
	for( iAStageView* sv : m_stages ) {
		data.push_back( *sv->getData() );
	}
	return data;*/
	return &m_data;
}

iAStageView * iA4DCTMainWin::addStage( iA4DCTStageData stageData )
{
	iA4DCTStageData * sd = new iA4DCTStageData( stageData );
	m_data.push_back( sd );
	iAStageView * stage = new iAStageView( );
	stage->setData( sd );
	stage->updateWidgets( );
	m_stages.push_back( stage );
	layoutStage->addWidget( (QWidget *)stage );
	return stage;
}

double * iA4DCTMainWin::getSize( )
{
	return m_size;
}

void iA4DCTMainWin::setSize( double * size )
{
	m_size[0] = size[0];
	m_size[1] = size[1];
	m_size[2] = size[2];
}

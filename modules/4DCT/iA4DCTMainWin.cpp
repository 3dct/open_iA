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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iA4DCTMainWin.h"
// iA
#include "dlg_findDefects.h"
#include "dlg_showDefects.h"
#include "iA4DCTData.h"
#include "iA4DCTDefectFinder.h"
#include "iA4DCTDefectView.h"
#include "iA4DCTDefects.h"
#include "iA4DCTProjectReaderWriter.h"
#include "iA4DCTSettings.h"
#include "iA4DCTVisWin.h"
#include "iAConsole.h"
#include "iAMhdFileInfo.h"
#include "iAPreviewMaker.h"
#include "iAStageView.h"
#include "mainwindow.h"
// itk
#include <itkImageFileReader.h>
#include <itkImageRegionIterator.h>
#include <itkImageFileWriter.h>
// Qt
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QListView>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QStringListModel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QXmlSimpleReader>
#include <QXmlStreamWriter>

iA4DCTMainWin::iA4DCTMainWin( MainWindow* parent /*= 0*/ )
	: QMainWindow( parent )
	, m_mainWnd( parent )
{
	setupUi( this );

	m_size[ 0 ] = 100.; m_size[ 1 ] = 100.; m_size[ 2 ] = 100.;

	connect( actionAdd, SIGNAL( triggered() ), this, SLOT( addButtonClick() ) );
	connect( actionDefectFinder, SIGNAL( triggered() ), this, SLOT( findDefects() ) );
	//connect(actionDefectView, SIGNAL(triggered()), this, SLOT(showDefects()));
	//connect(actionDensityMap, SIGNAL(triggered()), this, SLOT(densityMap()));
	connect( actionVisualization, SIGNAL( triggered() ), this, SLOT( openVisualizationWin() ) );
	connect( actionSave, SIGNAL( triggered() ), this, SLOT( save() ) );
}

iA4DCTMainWin::~iA4DCTMainWin()
{

}

void iA4DCTMainWin::load( QString settingsFile )
{
	iA4DCTProjectReaderWriter::load( this, settingsFile );
}

void iA4DCTMainWin::save()
{
	QSettings settings;
	QString fileName = QFileDialog::getSaveFileName(
		m_mainWnd,
		tr( "Save 4DCT proj" ),
		settings.value( S_4DCT_SAVE_DIR ).toString(),
		tr( "4DCT project (*.xml)" ) );

	QFileInfo file( fileName );
	if( !file.dir().exists() ) {
		return;
	}
	settings.setValue( S_4DCT_SAVE_DIR, file.absolutePath() );

	save( fileName );
}

void iA4DCTMainWin::save( QString settingsFile )
{
	iA4DCTProjectReaderWriter::save( this, settingsFile );
}

void iA4DCTMainWin::addButtonClick()
{
	// open dialog
	QSettings settings;
	QString fileNamePath = settings.value( S_4DCT_ADD_BUTTON_DLG ).toString();
	QString fileName = QFileDialog::getOpenFileName( m_mainWnd, "Open file", fileNamePath, "Meta header (*.mhd)" );
	if( !fileName.isEmpty() ) {
		settings.setValue( S_4DCT_ADD_BUTTON_DLG, fileName );
	}

	// does file exist
	QFileInfo fiMhd( fileName );
	if( !fiMhd.exists() )
		return;

	// add widget
	iA4DCTStageData stageData;
	stageData.Files.push_back( iA4DCTFileData( fiMhd.absoluteFilePath(), fiMhd.baseName() ) );
	iAPreviewMaker::makeUsingType( fiMhd.absoluteFilePath(), fiMhd.absolutePath() + "/thumbnail.png" );
	iA4DCTFileData file( fiMhd.absolutePath() + "/thumbnail.png", S_4DCT_THUMB_NAME );
	stageData.Files.push_back( file );
	addStage( stageData );

	if( m_stages.size() <= 1 )
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

void iA4DCTMainWin::openVisualizationWin()
{
	MainWindow * mainWin = qobject_cast< MainWindow* >( qApp->activeWindow() );
	iA4DCTVisWin* visWin = new iA4DCTVisWin( this );
	visWin->setImageSize( m_size );
	visWin->setNumberOfStages( m_stages.size() );
	mainWin->mdiArea->addSubWindow( visWin );
	visWin->showMaximized();
}

void iA4DCTMainWin::findDefects()
{
	dlg_findDefects* dialog = new dlg_findDefects( getStageData(), this );
	if( dialog->exec() != QDialog::Accepted ) {
		return;
	}

	iA4DCTStageData* stgData = m_stages[ dialog->getStageIndex() ]->getData();

	QString outputDir = dialog->getOutputDir();
	QDir dir( outputDir );
	if( !dir.exists() ) {
		DEBUG_LOG( "Wrong output directory" );
		return;
	}

	QString intensPath = stgData->Files[ dialog->getIntensityImgIndex() ].Path;
	QString labeldPath = stgData->Files[ dialog->getLabledImgIndex() ].Path;
	//QString fiberInfoPath	= stgData->ExtractedFibers;
	QString fiberInfoPath;
	if( !stgData->getFilePath( S_4DCT_EXTRACTED_FIBERS, fiberInfoPath ) )
		return;


	//typedef unsigned int	PixelType;
	//const int Dim = 3;
	typedef itk::Image<unsigned short, 3>		TImage;
	typedef itk::Image<unsigned short, 3>		TLabelImage;

	iA4DCTDefectFinder<TLabelImage, TImage> finder;
	finder.setFiberInfo( fiberInfoPath );
	finder.setIntensityImg( intensPath );
	finder.setLabeledImg( labeldPath );
	finder.run();

	iA4DCTDefects::save( finder.getBreakages(), outputDir + "/breakages.txt" );
	iA4DCTDefects::save( finder.getCracks(), outputDir + "/cracks.txt" );
	iA4DCTDefects::save( finder.getDebondings(), outputDir + "/debondings.txt" );
	iA4DCTDefects::save( finder.getPullouts(), outputDir + "/pullouts.txt" );

	// read image
	typedef itk::ImageFileReader<TImage>	ReaderType;
	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName( labeldPath.toStdString() );
	reader->Update();
	TImage::Pointer labelImg = reader->GetOutput();
	itk::SmartPointer<TImage> pullouts = TImage::New();
	pullouts->SetRegions( labelImg->GetLargestPossibleRegion() );
	pullouts->Allocate();
	itk::SmartPointer<TImage> breakages = TImage::New();
	breakages->SetRegions( labelImg->GetLargestPossibleRegion() );
	breakages->Allocate();
	itk::SmartPointer<TImage> cracks = TImage::New();
	cracks->SetRegions( labelImg->GetLargestPossibleRegion() );
	cracks->Allocate();
	itk::SmartPointer<TImage> debondings = TImage::New();
	debondings->SetRegions( labelImg->GetLargestPossibleRegion() );
	debondings->Allocate();

	iA4DCTDefects::HashDataType breakagesHash = iA4DCTDefects::DefectDataToHash( finder.getBreakages() );
	iA4DCTDefects::HashDataType pulloutsHash = iA4DCTDefects::DefectDataToHash( finder.getPullouts() );
	iA4DCTDefects::HashDataType cracksHash = iA4DCTDefects::DefectDataToHash( finder.getCracks() );
	iA4DCTDefects::HashDataType debondingHash = iA4DCTDefects::DefectDataToHash( finder.getDebondings() );

	itk::ImageRegionIterator<TImage> labelIterator( labelImg, labelImg->GetLargestPossibleRegion() );
	itk::ImageRegionIterator<TImage> breakageIterator( breakages, breakages->GetLargestPossibleRegion() );
	itk::ImageRegionIterator<TImage> pulloutIterator( pullouts, pullouts->GetLargestPossibleRegion() );
	itk::ImageRegionIterator<TImage> crackIterator( cracks, cracks->GetLargestPossibleRegion() );
	itk::ImageRegionIterator<TImage> debondingIterator( debondings, debondings->GetLargestPossibleRegion() );
	while(!labelIterator.IsAtEnd())
    {
		if( breakagesHash.contains( labelIterator.Get() ) )	{
			breakageIterator.Set(255); 
		}
		else {
			breakageIterator.Set(0); 
		}

		if( pulloutsHash.contains( labelIterator.Get() ) )	{
			pulloutIterator.Set(255); 
		}
		else {
			pulloutIterator.Set(0); 
		}

		if( cracksHash.contains( labelIterator.Get() ) )	{
			crackIterator.Set(255); 
		}
		else {
			crackIterator.Set(0); 
		}

		if( debondingHash.contains( labelIterator.Get() ) )	{
			debondingIterator.Set(255); 
		}
		else {
			debondingIterator.Set(0); 
		}

		++labelIterator;
		++breakageIterator;
		++pulloutIterator;
		++crackIterator;
		++debondingIterator;
    }

	typedef itk::ImageFileWriter<TImage> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName( outputDir.toStdString() + "/breakages.mhd" );
	writer->SetInput( breakages );
	writer->Update();
	writer->SetFileName( outputDir.toStdString() + "/debondings.mhd" );
	writer->SetInput( debondings );
	writer->Update();
	writer->SetFileName( outputDir.toStdString() + "/pullouts.mhd" );
	writer->SetInput( pullouts );
	writer->Update();
	writer->SetFileName( outputDir.toStdString() + "/cracks.mhd" );
	writer->SetInput( cracks );
	writer->Update();
}

void iA4DCTMainWin::showDefects()
{
	dlg_showDefects* dialog = new dlg_showDefects( getStageData(), this );
	if( dialog->exec() != QDialog::Accepted ) {
		return;
	}

	iA4DCTStageData* sData = m_stages[ dialog->getStageIndex() ]->getData();
	QString intensImgPath = sData->Files[ dialog->getIntensityImgIndex() ].Path;
	QString labeledImgPath = sData->Files[ dialog->getLabledImgIndex() ].Path;
	QString pulloutsPath = sData->Files[ dialog->getPulloutsFileIndex() ].Path;
	QString cracksPath = sData->Files[ dialog->getCracksFileIndex() ].Path;
	QString breakagesPath = sData->Files[ dialog->getBreakagesFileIndex() ].Path;
	QString debondingsPath = sData->Files[ dialog->getDebondingsFileIndex() ].Path;

	MainWindow* mainWin = qobject_cast< MainWindow* >( qApp->activeWindow() );
	iA4DCTDefectView* defectView = new iA4DCTDefectView( mainWin );
	mainWin->mdiArea->addSubWindow( defectView );
	defectView->show();

	defectView->initializeSlicer( intensImgPath );
	defectView->setDefects( labeledImgPath, pulloutsPath, cracksPath, breakagesPath, debondingsPath );
}

void iA4DCTMainWin::densityMap()
{ /* not yet implemented */ }

iA4DCTData * iA4DCTMainWin::getStageData()
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
	iAStageView * stage = new iAStageView();
	stage->setData( sd );
	stage->updateWidgets();
	m_stages.push_back( stage );
	layoutStage->addWidget( ( QWidget * )stage );
	return stage;
}

double * iA4DCTMainWin::getSize()
{
	return m_size;
}

void iA4DCTMainWin::setSize( double * size )
{
	m_size[ 0 ] = size[ 0 ];
	m_size[ 1 ] = size[ 1 ];
	m_size[ 2 ] = size[ 2 ];
}

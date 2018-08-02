/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAPorosityAnalyser.h"

#include "defines.h"
#include "iACSVToQTableWidgetConverter.h"
#include "io/iAITKIO.h"
//#include "iAPCView.h"
#include "iAPDMView.h"
#include "iAPreviewSPLOMView.h"
#include "iARangeSliderDiagramView.h"
#include "iASegm3DView.h"
#include "iASelectionsView.h"
#include "iASPMView.h"
#include "iASSView.h"
#include "iATreeView.h"
#include "PorosityAnalyserHelpers.h"

#include <vtkIdTypeArray.h>
#include <vtkSelection.h>

#include <QDir>
#include <QCheckBox>
#include <QGroupBox>
#include <QMenu>
#include <QDockWidget>
#include <QDirIterator>
#include <QSettings>
#include <QTreeWidget>
#include <QStatusBar>

const int treeViewIndex = 0;
const int overviewIndex = 1;

iAPorosityAnalyser::iAPorosityAnalyser(MainWindow *mWnd, const QString & resDir, const QString & datasetsDir, QWidget * parent /*= 0*/, Qt::WindowFlags f /*= 0 */ ) : PorosityAnalyserConnector( parent, f ),
	m_dataDir( resDir ),
	m_datasetsDir( datasetsDir ),
	m_spmView( new iASPMView(mWnd, parent, f ) ),
	m_treeView( new iATreeView( 0, f ) ),
	m_pdmView( new iAPDMView( parent, f ) ),
	//m_pcView( new iAPCView( parent, f ) ),
	m_ssView( new iASSView( parent, f ) ),
	m_rangeSliderDiagramView( new iARangeSliderDiagramView( parent, f ) ),
	m_selView( new iASelectionsView( 0, f ) ),
	m_segm3DView( new iASegm3DView( parent, f ) ),
	m_prvSplomView( new iAPreviewSPLOMView( parent, f ) ),
	m_runsOffset( -1 ),
	m_visanMW( new QMainWindow( 0 ) )
{
	vtkObject::GlobalWarningDisplayOff();
	//prepare window for handling dock widgets
	m_visanMW->setCentralWidget( 0 );
	m_visanMW->setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
	m_visanMW->setParent( mainArea );

	QVBoxLayout * visanLayout = new QVBoxLayout();
	visanLayout->setMargin( 0 );	
	visanLayout->setSpacing( 0 );
	visanLayout->addWidget( m_visanMW );
	mainArea->setLayout( visanLayout );

	selectionsExplorer->hide();

	const int defaultPopupSizePercentage = 20;
	m_prvSplomView->sliderPreviewSize->setValue( defaultPopupSizePercentage );
	m_spmView->setSPLOMPreviewSize( defaultPopupSizePercentage );

	connect( m_treeView, SIGNAL( loadSelectionToSPMSignal( const QTableWidget* ) ), m_spmView, SLOT( SetData( const QTableWidget* ) ) );
	connect( m_treeView, SIGNAL( loadSelectionToSSSignal( const QTableWidget*, QString ) ), m_ssView, SLOT( SetData( const QTableWidget*, QString ) ) );
	connect( m_treeView, SIGNAL( loadSelectionsToSSSignal( const QList< QPair<QTableWidget *, QString> > * ) ), m_ssView, SLOT( SetCompareData( const QList< QPair<QTableWidget *, QString> > * ) ) );
	connect( m_treeView, SIGNAL( loadSelectionToPDMSignal( const iABPMData*, const iAHMData* ) ), m_pdmView, SLOT( SetData( const iABPMData*, const iAHMData* ) ) );
	//connect( m_treeView, SIGNAL( loadSelectionToPCSignal( const QTableWidget* ) ), m_pcView, SLOT( SetData( const QTableWidget* ) ) );
	connect( m_treeView, SIGNAL( loadSelectionToRSDSignal( const QTableWidget* ) ), m_rangeSliderDiagramView, SLOT( setData( const QTableWidget* ) ) );
	connect( tbSelections, SIGNAL( clicked( bool ) ), this, SLOT( ShowSelections( bool ) ) );
	connect( tbTreeView, SIGNAL( clicked( bool ) ), this, SLOT( ShowTreeView( bool ) ) );
	connect( m_treeView, SIGNAL( selectionModified( QList<QTreeWidgetItem*>* ) ), m_selView, SLOT( selectionModifiedTreeView( QList<QTreeWidgetItem*>* ) ) );
	connect( m_spmView, SIGNAL( selectionModified( vtkVector2i, vtkIdTypeArray* ) ), m_selView, SLOT( selectionModifiedSPMView( vtkVector2i, vtkIdTypeArray* ) ) );
	connect( m_pdmView, SIGNAL( selectionModified( QModelIndexList ) ), m_selView, SLOT( selectionModifiedPDMView( QModelIndexList ) ) );
	connect( m_selView, SIGNAL( loadedSelection( iASelection* ) ), this, SLOT( selectionLoaded( iASelection* ) ) );
	connect( m_selView, SIGNAL( visualizeSelection( iASelection* ) ), m_treeView, SLOT( loadSelectionToSS( iASelection* ) ) );
	connect( m_selView, SIGNAL( compareSelections( QList<iASelection*> ) ), m_treeView, SLOT( loadSelectionsToSS( QList<iASelection*> ) ) );
	connect( this, SIGNAL( loadTreeDataToViews() ), m_treeView, SLOT( loadFilteredDataToOverview() ) );
	connect( m_pdmView, SIGNAL( selectionModified( QModelIndexList ) ), m_treeView, SLOT( loadOverviewSelectionToSPM( QModelIndexList ) ) );
	connect( m_rangeSliderDiagramView, SIGNAL( selectionModified( vtkIdTypeArray* ) ), m_spmView, SLOT( setRSDSelection( vtkIdTypeArray* ) ) );
	connect( m_treeView, SIGNAL( clearOldRSDViewSignal() ), m_rangeSliderDiagramView, SLOT( clearOldRSDView() ) );
	connect( this, SIGNAL( runsOffsetChanged( int ) ), m_ssView, SLOT( setRunsOffset( int ) ) );
	connect( m_treeView, SIGNAL( displayMessage( QString ) ), this, SLOT( message( QString ) ) );
	connect( m_treeView, SIGNAL( loadDatasetsToPreviewSignal( QStringList ) ), m_prvSplomView, SLOT( SetDatasets( QStringList ) ) );
	connect( m_treeView, SIGNAL( loadAllDatasetsByIndicesSignal( QStringList, QList<int> ) ), m_spmView, SLOT( setDatasetsByIndices( QStringList, QList<int> ) ) );
	connect( m_spmView, SIGNAL( previewSliceChanged( int ) ), m_prvSplomView, SLOT( SetSlice( int ) ) );
	connect( m_spmView, SIGNAL( sliceCountChanged( int ) ), m_prvSplomView, SLOT( SetSliceCount( int ) ) );
	connect( m_spmView, SIGNAL( maskHovered( const QPixmap *, int ) ), m_prvSplomView, SLOT( SetMask( const QPixmap *, int ) ) );
	connect( m_prvSplomView, SIGNAL( sliceNumbersChanged( QList<int> ) ), m_spmView, SLOT( setSPLOMPreviewSliceNumbers( QList<int> ) ) );
	connect( m_prvSplomView, SIGNAL( roiChanged( QList<QRectF> ) ), m_spmView, SLOT( setROIList( QList<QRectF> ) ) );
	connect( m_prvSplomView, SIGNAL( sliceCountsChanged( QList<int> ) ), m_spmView, SLOT( setSliceCnts( QList<int> ) ) );
	connect( m_prvSplomView->sliderPreviewSize, SIGNAL( valueChanged( int ) ), m_spmView, SLOT( setSPLOMPreviewSize( int ) ) );
	connect( m_prvSplomView->cbDatasets, SIGNAL( currentIndexChanged( int ) ), m_spmView, SLOT( reemitFixedPixmap() ) );

	m_visanMW->addDockWidget( Qt::LeftDockWidgetArea, m_pdmView );
	m_visanMW->splitDockWidget( m_pdmView, m_rangeSliderDiagramView, Qt::Horizontal );
	m_visanMW->addDockWidget( Qt::RightDockWidgetArea, m_prvSplomView );
	m_visanMW->splitDockWidget( m_rangeSliderDiagramView, m_spmView, Qt::Vertical );
	m_visanMW->splitDockWidget( m_prvSplomView, m_ssView, Qt::Vertical );
	m_visanMW->tabifyDockWidget( m_ssView, m_segm3DView );
	m_ssView->attachSegm3DView( m_segm3DView );
	
	m_rangeSliderDiagramView->raise();
	//m_rangeSliderDiagramView->hide();

	m_spmView->raise();
	m_ssView->raise();

	m_prvSplomView->SetDatasetsDir( m_datasetsDir );
	m_spmView->setDatasetsDir( m_datasetsDir );

	LoadData();
	m_treeView->SetData( &m_data, &m_gtPorosityMap, m_runsOffset );
}

iAPorosityAnalyser::~iAPorosityAnalyser()
{
	QByteArray state = saveState( 0 );
	QSettings settings( organisationName, applicationName );
	settings.setValue( "PorosityAnalyser/UI_State", state );
}

void iAPorosityAnalyser::CalculateRunsOffset()
{
	if( m_dataDir == "" )
		return;
	m_data.clear();
	QDir dataRootDir( m_dataDir );
	dataRootDir.setFilter( QDir::Dirs );
	QStringList dirs = dataRootDir.entryList();
	foreach( QString d, dirs )
	{
		if( d == "." || d == ".." )
			continue;
		QString subDirName = m_dataDir + "/" + d;
		QDir subDir( subDirName );
		subDir.setFilter( QDir::Files );
		QStringList files = subDir.entryList();
		foreach( QString f, files )
		{
			QFileInfo fi( subDirName + "/" + f );
			if( QString::compare( fi.suffix(), "CSV", Qt::CaseInsensitive ) == 0 )
			{
				QString absPath = fi.absolutePath();
				QTableWidget compCSV;
				iACSVToQTableWidgetConverter::loadCSVFile( fi.absoluteFilePath(), &compCSV );
				//calculate maximum number of columns in any batches.csv for the proper alignment
				int maxBatchesColumnCount = 0;
				for( int cid = 1; cid < compCSV.rowCount(); ++cid ) //1 because 0 is header
				{
					QString batchesCSVDirPath = absPath + "/" + compCSV.item( cid, 4 )->text();
					int batchesCSVColCount = iACSVToQTableWidgetConverter::getCSVFileColumnCount( batchesCSVDirPath + "/batches.csv" );
					if( batchesCSVColCount > maxBatchesColumnCount )
						maxBatchesColumnCount = batchesCSVColCount;
				}
				int newRunsOffset = compCSV.columnCount() + maxBatchesColumnCount;
				if( newRunsOffset > m_runsOffset )
					m_runsOffset = newRunsOffset;
			}
		}
	}
	emit runsOffsetChanged( m_runsOffset );
}

void iAPorosityAnalyser::LoadData()
{
	CalculateRunsOffset();
	if( m_dataDir == "" )
		return;
	m_referenceData.clear();
	//qDebug() << m_dataDir + "/DatasetDescription.csv";
	iACSVToQTableWidgetConverter::loadCSVFile( m_dataDir + "/DatasetDescription.csv", &m_referenceData );
	for( int i = 1; i < m_referenceData.rowCount(); i++ )
		m_gtPorosityMap[m_referenceData.item( i, gtDatasetColInd )->text()] = m_referenceData.item( i, gtPorosityColInd )->text().toDouble();
		
	m_data.clear();
	QDir dataRootDir( m_dataDir );
	dataRootDir.setFilter( QDir::Dirs );
	QStringList dirs = dataRootDir.entryList();
	foreach( QString d, dirs )
	{
		if( d == "." || d == ".." )
			continue;
		AddSubdirectory( m_dataDir + "/" + d );
	}
}

void iAPorosityAnalyser::AddSubdirectory( const QString & subDirName )
{
	QDir subDir( subDirName );
	subDir.setFilter( QDir::Files );
	QStringList files = subDir.entryList();
	foreach( QString f, files )
	{
		QFileInfo fi( subDirName + "/" + f );
		if( QString::compare( fi.suffix(), "CSV", Qt::CaseInsensitive ) == 0 )
			ParseComputerCSV( fi );
	}
}

void iAPorosityAnalyser::ParseComputerCSV( const QFileInfo & fi )
{
	QString absPath = fi.absolutePath();
	QTableWidget compCSV;
	iACSVToQTableWidgetConverter::loadCSVFile( fi.absoluteFilePath(), &compCSV );

	for( int cid = 1; cid < compCSV.rowCount(); ++cid ) //1 because 0 is header
	{
		QStringList compData;
		for( int i = 0; i < compCSV.columnCount(); ++i )
			compData << compCSV.item( cid, i )->text();

		QString batchesCSVDirPath = absPath + "/" + compCSV.item( cid, 4 )->text();
		QTableWidget batchesCSV;
		iACSVToQTableWidgetConverter::loadCSVFile( batchesCSVDirPath + "/batches.csv", &batchesCSV );
		for( int bid = 1; bid < batchesCSV.rowCount(); ++bid ) //1 because 0 is header
		{
			QStringList batchesData;
			for( int i = 0; i < batchesCSV.columnCount(); ++i )
				batchesData << batchesCSV.item( bid, i )->text();

			QString runsCSVDirPath = batchesCSVDirPath + "/batch" + QString::number( bid ) + "/runs.csv";
			QTableWidget runsCSV;
			iACSVToQTableWidgetConverter::loadCSVFile( runsCSVDirPath, &runsCSV );
			int colCnt = m_runsOffset + runsCSV.columnCount();
			if( m_data.columnCount() < colCnt )
				m_data.setColumnCount( colCnt );
 			/////UNCOMMENT TO CALCULATE DICE METRIC/////////////////////////////////////
 			//QMap<QString, QString> datasetGTs;
 			//for( int i = 1; i < m_referenceData.rowCount(); i++ )
 			//	datasetGTs[m_referenceData.item( i, gtDatasetColInd )->text()] = m_referenceData.item( i, gtGTSegmColumnIndex )->text();
 			//QString datasetName = compCSV.item( cid, 3 )->text();
 			//QString gtMaskFile = m_datasetsDir + "/" + datasetGTs[datasetName];
 			//ScalarPixelType maskPixType;
 			//ImagePointer gtMaskPtr = iAITKIO::readFile( gtMaskFile, maskPixType, true );
 			////////////////////////////////////////////////////////////////////////////
			for( int rid = 1; rid < runsCSV.rowCount(); ++rid ) //1 because 0 is header
			{
				int lastRow = m_data.rowCount(), col = 0;
				m_data.insertRow( lastRow );
				//add info from computer, batches, and runs CSVs
				for( int i = 0; i < compData.size(); i++ )
					m_data.setItem( lastRow, col++, new QTableWidgetItem( compData[i] ) );
				for( int i = 0; i < batchesData.size(); i++ )
					m_data.setItem( lastRow, col++, new QTableWidgetItem( batchesData[i] ) );
				while( col < m_runsOffset )//fill the alignment with empty if needed
					m_data.setItem( lastRow, col++, new QTableWidgetItem( "" ) );
				for( int i = 0; i < runsCSV.columnCount(); i++ )
					m_data.setItem( lastRow, col++, new QTableWidgetItem( runsCSV.item( rid, i )->text() ) );
				//substitute relative path with global path for the mask file
				int maskInd = m_runsOffset + maskOffsetInRuns;
				QString fullMaskPath = batchesCSVDirPath + "/batch" + QString::number( bid ) + "/masks/" + m_data.item( lastRow, maskInd )->text();
				m_data.item( lastRow, maskInd )->setText( fullMaskPath );
 //				///UNCOMMENT TO CALCULATE DICE METRIC/////////////////////////////////////
 //				ScalarPixelType maskPixType;
 //				ImagePointer maskPtr = iAITKIO::readFile( fullMaskPath, maskPixType, true );
 //				MaskImageType * mask = dynamic_cast<MaskImageType*>(maskPtr.GetPointer());
	//			MaskImageType * gtImage = dynamic_cast<MaskImageType*>( gtMaskPtr.GetPointer() );
 //				MaskImageType::RegionType reg = mask->GetLargestPossibleRegion();
 //				int size = reg.GetSize()[0] * reg.GetSize()[1] * reg.GetSize()[2];
 //				float fpe = 0.0f, fne = 0.0f, totalGT = 0.0f;
 //#pragma omp parallel for reduction(+:fpe,fne,totalGT)
 //				for( int i = 0; i < size; ++i )
 //				{
 //					if( gtImage->GetBufferPointer()[i] )
 //						++totalGT;
 //					if( !gtImage->GetBufferPointer()[i] && mask->GetBufferPointer()[i] )
 //						++fpe;
 //					if( gtImage->GetBufferPointer()[i] && !mask->GetBufferPointer()[i] )
 //						++fne;
 //				}
	//			fpe /= totalGT;
 //				fne /= totalGT;
 //				int errorInd = paramsOffsetInRunsCSV - 2;
 //				//QString oldfpe = runsCSV.item( rid, errorInd )->text();
 //				//QString oldfne = runsCSV.item( rid, errorInd + 1 )->text();
 //				runsCSV.item( rid, errorInd )->setText( QString::number( fpe ) );
 //				runsCSV.item( rid, errorInd + 1 )->setText( QString::number( fne ) );
 				//////////////////////////////////////////////////////////////////////////
			}
 			/////UNCOMMENT TO CALCULATE DICE METRIC/////////////////////////////////////
 			//iACSVToQTableWidgetConverter::saveToCSVFile( runsCSV, runsCSVDirPath );
 			////////////////////////////////////////////////////////////////////////////
		}
	}
}

void iAPorosityAnalyser::LoadStateAndShow()
{
	QSettings settings( organisationName, applicationName );
	QByteArray state = settings.value( "PorosityAnalyser/UI_State" ).value<QByteArray>();
	showMaximized();
	restoreState( state, 0 );
}

void iAPorosityAnalyser::ShowSelections( bool checked )
{
	//TODO: bad code
	if( checked )
	{
		tbTreeView->setChecked( false );
		m_treeView->hide();

		m_selView->setParent( selectionsExplorer );
		QVBoxLayout * layout = new QVBoxLayout();
		layout->setMargin( 0 );
		layout->setSpacing( 0 );
		layout->addWidget( m_selView );
		delete selectionsExplorer->layout();
		selectionsExplorer->setLayout( layout );
		m_selView->show();
		selectionsExplorer->show();
	}
	else
		selectionsExplorer->hide();
}

void iAPorosityAnalyser::ShowTreeView( bool checked )
{
	//TODO: bad code
	if( checked )
	{
		tbSelections->setChecked( false );
		m_selView->hide();

		m_treeView->setParent( selectionsExplorer );
		QVBoxLayout * layout = new QVBoxLayout();
		layout->setMargin( 0 );
		layout->setSpacing( 0 );
		layout->addWidget( m_treeView );
		delete selectionsExplorer->layout();
		selectionsExplorer->setLayout( layout );
		m_treeView->show();
		selectionsExplorer->show();
	}
	else
		selectionsExplorer->hide();
}

void iAPorosityAnalyser::selectionLoaded( iASelection * sel )
{
	m_treeView->setSelection( &sel->treeItems );
	m_treeView->loadFilteredDataToOverview();
	m_pdmView->setSelection( sel->pdmSelection );
	loadOverviewSelectionToSPM( m_pdmView->SelectedIndices() );
	//m_spmView->SetData( m_treeView->GetSPMData() );
	m_spmView->setSelection( sel );
}

void iAPorosityAnalyser::tabChanged( int index )
{
	//emit loadTreeDataToViews(); 
}

void iAPorosityAnalyser::message( QString text )
{
	statusBar()->showMessage( text );
}

//void iAPorosityAnalyser::GenerateMasksData()
//{
// 	QTableWidget masksData;
//	masksData.setColumnCount( 1 );
//	masksData.setRowCount( 0 );
//
//	QDirIterator dirIt( m_dataDir, QDirIterator::Subdirectories );
//	while( dirIt.hasNext() )
//	{
//		dirIt.next();
//		QFileInfo fi = QFileInfo( dirIt.filePath() );
//		if( fi.isFile() )
//			if( QString::compare( fi.suffix(), "mhd", Qt::CaseInsensitive ) == 0 )
//			{
//				QFileInfo maskCSVFile( fi.absoluteFilePath() + ".csv" );
//				if( maskCSVFile.exists() )
//					continue;
//				int lastRow = masksData.rowCount();
//				masksData.insertRow( lastRow );
//				masksData.setItem( lastRow, 0, new QTableWidgetItem( fi.absoluteFilePath() ) );
//			}
//	}
//	iACSVToQTableWidgetConverter::saveToCSVFile( masksData, "C:/Users/p41036/Desktop/masks.csv" );
//}
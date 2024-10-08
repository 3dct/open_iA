// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureAnalyzer.h"

#include "iAFeatureAnalyzerTool.h"
#include "iAPDMView.h"
#include "iAPreviewSPLOMView.h"
#include "iARangeSliderDiagramView.h"
#include "iASegm3DView.h"
#include "iASelectionsView.h"
#include "iASPMView.h"
#include "iASSView.h"
#include "iATreeView.h"
#include "FeatureAnalyzerHelpers.h"

#include <iADockWidgetWrapper.h>
#include <iALog.h>
#include <iACSVToQTableWidgetConverter.h>
#include <iAITKIO.h>

#include <iAProjectFileIO.h>

#include <vtkIdTypeArray.h>

#include <QCheckBox>
#include <QDir>
#include <QDirIterator>
#include <QDockWidget>
#include <QGroupBox>
#include <QMenu>
#include <QSettings>
#include <QStatusBar>
#include <QTreeWidget>

iAFeatureAnalyzer::iAFeatureAnalyzer(iAMainWindow* mWnd, const QString& resDir, const QString& datasetsDir, QWidget* parent):
	FeatureAnalyzerConnector(parent),
	m_dataDir( resDir ),
	m_datasetsDir( datasetsDir ),
	m_spmView( new iASPMView(mWnd, parent) ),
	m_treeView( new iATreeView( nullptr ) ),
	m_pdmView( new iAPDMView(parent) ),
	m_ssView( new iASSView(parent) ),
	m_selView( new iASelectionsView( nullptr ) ),
	m_segm3DView( new iASegm3DView(parent) ),
	m_prvSplomView( new iAPreviewSPLOMView(parent) ),
	m_runsOffset( -1 ),
	m_visanMW( new QMainWindow( nullptr ) )
{
	vtkObject::GlobalWarningDisplayOff();
	//prepare window for handling dock widgets
	m_visanMW->setCentralWidget(nullptr);
	m_visanMW->setTabPosition( Qt::AllDockWidgetAreas, QTabWidget::North );
	m_visanMW->setParent( mainArea );

	QVBoxLayout * visanLayout = new QVBoxLayout();
	visanLayout->setContentsMargins(0, 0, 0, 0);
	visanLayout->setSpacing( 0 );
	visanLayout->addWidget( m_visanMW );
	mainArea->setLayout( visanLayout );

	selectionsExplorer->hide();

	const int defaultPopupSizePercentage = 20;
	m_prvSplomView->sliderPreviewSize->setValue( defaultPopupSizePercentage );
	m_spmView->setSPLOMPreviewSize(defaultPopupSizePercentage);

	iARangeSliderDiagramView * rangeSliderDiagramView = new iARangeSliderDiagramView(parent);

	connect( m_treeView, &iATreeView::loadSelectionToSPMSignal, m_spmView, &iASPMView::setData);
	connect( m_treeView, &iATreeView::loadSelectionToSSSignal, m_ssView, &iASSView::SetData);
	connect( m_treeView, &iATreeView::loadSelectionsToSSSignal, m_ssView, &iASSView::SetCompareData);
	connect( m_treeView, &iATreeView::loadSelectionToPDMSignal, m_pdmView, &iAPDMView::SetData);
	connect( m_treeView, &iATreeView::loadSelectionToRSDSignal, rangeSliderDiagramView, &iARangeSliderDiagramView::setData);
	connect( tbSelections, &QToolButton::clicked, this, &iAFeatureAnalyzer::ShowSelections);
	connect( tbTreeView, &QToolButton::clicked, this, &iAFeatureAnalyzer::ShowTreeView);
	connect( m_treeView, &iATreeView::selectionModified, m_selView, &iASelectionsView::selectionModifiedTreeView);
	connect( m_spmView, &iASPMView::selectionModified, m_selView, &iASelectionsView::selectionModifiedSPMView);
	connect( m_pdmView, &iAPDMView::selectionModified, m_selView, &iASelectionsView::selectionModifiedPDMView);
	connect( m_selView, &iASelectionsView::loadedSelection, this, &iAFeatureAnalyzer::selectionLoaded);
	connect( m_selView, &iASelectionsView::visualizeSelection, m_treeView, QOverload<iASelection*>::of(&iATreeView::loadSelectionToSS));
	connect( m_selView, &iASelectionsView::compareSelections, m_treeView, &iATreeView::loadSelectionsToSS);
	connect( this, &iAFeatureAnalyzer::loadTreeDataToViews, m_treeView, &iATreeView::loadFilteredDataToOverview);
	connect( m_pdmView, &iAPDMView::selectionModified, m_treeView, &iATreeView::loadOverviewSelectionToSPM);
	connect( rangeSliderDiagramView, &iARangeSliderDiagramView::selectionModified, m_spmView, &iASPMView::setRSDSelection);
	connect( m_treeView, &iATreeView::clearOldRSDViewSignal, rangeSliderDiagramView, &iARangeSliderDiagramView::clearOldRSDView);
	connect( this, &iAFeatureAnalyzer::runsOffsetChanged, m_ssView, &iASSView::setRunsOffset);
	connect( m_treeView, &iATreeView::loadDatasetsToPreviewSignal, m_prvSplomView, &iAPreviewSPLOMView::SetDatasets);
	connect( m_treeView, &iATreeView::loadAllDatasetsByIndicesSignal, m_spmView, &iASPMView::setDatasetsByIndices);
	connect( m_spmView, &iASPMView::previewSliceChanged, m_prvSplomView, &iAPreviewSPLOMView::SetSlice);
	connect( m_spmView, &iASPMView::sliceCountChanged, m_prvSplomView, &iAPreviewSPLOMView::SetSliceCount);
	connect( m_spmView, &iASPMView::maskHovered, m_prvSplomView, &iAPreviewSPLOMView::SetMask);
	connect( m_prvSplomView, &iAPreviewSPLOMView::sliceNumbersChanged, m_spmView, &iASPMView::setSPLOMPreviewSliceNumbers);
	connect( m_prvSplomView, &iAPreviewSPLOMView::roiChanged, m_spmView, &iASPMView::setROIList);
	connect( m_prvSplomView, &iAPreviewSPLOMView::sliceCountsChanged, m_spmView, &iASPMView::setSliceCnts);
	connect( m_prvSplomView->sliderPreviewSize, &QSlider::valueChanged, m_spmView, &iASPMView::setSPLOMPreviewSize);
	connect( m_prvSplomView->cbDatasets, QOverload<int>::of(&QComboBox::currentIndexChanged), m_spmView, &iASPMView::reemitFixedPixmap);

	m_visanMW->addDockWidget( Qt::LeftDockWidgetArea, m_pdmView );
	auto dwRangeSliderChart =
		new iADockWidgetWrapper(rangeSliderDiagramView, "Parameter Range Slider", "RangeSliderDiagramView", "https://github.com/3dct/open_iA/wiki/FeatureAnalyzer");
	m_visanMW->splitDockWidget(m_pdmView, dwRangeSliderChart, Qt::Horizontal);
	m_visanMW->addDockWidget( Qt::RightDockWidgetArea, m_prvSplomView );
	m_visanMW->splitDockWidget(dwRangeSliderChart, m_spmView, Qt::Vertical);
	m_visanMW->splitDockWidget( m_prvSplomView, m_ssView, Qt::Vertical );
	m_visanMW->tabifyDockWidget( m_ssView, m_segm3DView );
	m_ssView->attachSegm3DView( m_segm3DView );

	dwRangeSliderChart->raise();

	m_spmView->raise();
	m_ssView->raise();

	m_prvSplomView->SetDatasetsDir( m_datasetsDir );
	m_spmView->setDatasetsDir( m_datasetsDir );

	LoadData();
	m_treeView->SetData( &m_data, &m_gtPorosityMap, m_runsOffset );
}

iAFeatureAnalyzer::~iAFeatureAnalyzer()
{
	QByteArray state = saveState( 0 );
	QSettings settings;
	settings.setValue( "FeatureAnalyzer/UI_State", state );
}

void iAFeatureAnalyzer::CalculateRunsOffset()
{
	if (m_dataDir == "")
	{
		return;
	}
	m_data.clear();
	QDir dataRootDir( m_dataDir );
	dataRootDir.setFilter( QDir::Dirs );
	QStringList dirs = dataRootDir.entryList();
	for (QString d: dirs)
	{
		if (d == "." || d == "..")
		{
			continue;
		}
		QString subDirName = m_dataDir + "/" + d;
		QDir subDir( subDirName );
		subDir.setFilter( QDir::Files );
		QStringList files = subDir.entryList();
		for (QString f: files)
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
					if (batchesCSVColCount > maxBatchesColumnCount)
					{
						maxBatchesColumnCount = batchesCSVColCount;
					}
				}
				int newRunsOffset = compCSV.columnCount() + maxBatchesColumnCount;
				if (newRunsOffset > m_runsOffset)
				{
					m_runsOffset = newRunsOffset;
				}
			}
		}
	}
	emit runsOffsetChanged( m_runsOffset );
}

void iAFeatureAnalyzer::LoadData()
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
	for (QString d: dirs)
	{
		if (d == "." || d == "..")
		{
			continue;
		}
		AddSubdirectory( m_dataDir + "/" + d );
	}
}

void iAFeatureAnalyzer::AddSubdirectory( const QString & subDirName )
{
	QDir subDir( subDirName );
	subDir.setFilter( QDir::Files );
	QStringList files = subDir.entryList();
	for (QString f: files)
	{
		QFileInfo fi( subDirName + "/" + f );
		if (QString::compare(fi.suffix(), "CSV", Qt::CaseInsensitive) == 0)
		{
			ParseComputerCSV(fi);
		}
	}
}

void iAFeatureAnalyzer::ParseComputerCSV( const QFileInfo & fi )
{
	QString absPath = fi.absolutePath();
	QTableWidget compCSV;
	iACSVToQTableWidgetConverter::loadCSVFile( fi.absoluteFilePath(), &compCSV );

	for( int cid = 1; cid < compCSV.rowCount(); ++cid ) //1 because 0 is header
	{
		QStringList compData;
		for (int i = 0; i < compCSV.columnCount(); ++i)
		{
			compData << compCSV.item(cid, i)->text();
		}

		QString batchesCSVDirPath = absPath + "/" + compCSV.item( cid, 4 )->text();
		QTableWidget batchesCSV;
		iACSVToQTableWidgetConverter::loadCSVFile( batchesCSVDirPath + "/batches.csv", &batchesCSV );
		for( int bid = 1; bid < batchesCSV.rowCount(); ++bid ) //1 because 0 is header
		{
			QStringList batchesData;
			for (int i = 0; i < batchesCSV.columnCount(); ++i)
			{
				batchesData << batchesCSV.item(bid, i)->text();
			}

			QString runsCSVDirPath = batchesCSVDirPath + "/batch" + QString::number( bid ) + "/runs.csv";
			QTableWidget runsCSV;
			iACSVToQTableWidgetConverter::loadCSVFile( runsCSVDirPath, &runsCSV );
			int colCnt = m_runsOffset + runsCSV.columnCount();
			if (m_data.columnCount() < colCnt)
			{
				m_data.setColumnCount(colCnt);
			}
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
				for (int i = 0; i < compData.size(); i++)
				{
					m_data.setItem(lastRow, col++, new QTableWidgetItem(compData[i]));
				}
				for (int i = 0; i < batchesData.size(); i++)
				{
					m_data.setItem(lastRow, col++, new QTableWidgetItem(batchesData[i]));
				}
				while (col < m_runsOffset)//fill the alignment with empty if needed
				{
					m_data.setItem(lastRow, col++, new QTableWidgetItem(""));
				}
				for (int i = 0; i < runsCSV.columnCount(); i++)
				{
					m_data.setItem(lastRow, col++, new QTableWidgetItem(runsCSV.item(rid, i)->text()));
				}
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

void iAFeatureAnalyzer::LoadStateAndShow()
{
	QSettings settings;
	QByteArray state = settings.value( "FeatureAnalyzer/UI_State" ).value<QByteArray>();
	showMaximized();
	restoreState( state, 0 );
}

void iAFeatureAnalyzer::ShowSelections( bool checked )
{
	//TODO: bad code
	if (checked)
	{
		tbTreeView->setChecked( false );
		m_treeView->hide();

		m_selView->setParent( selectionsExplorer );
		QVBoxLayout * layout = new QVBoxLayout();
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing( 0 );
		layout->addWidget( m_selView );
		delete selectionsExplorer->layout();
		selectionsExplorer->setLayout( layout );
		m_selView->show();
		selectionsExplorer->show();
	}
	else
	{
		selectionsExplorer->hide();
	}
}

void iAFeatureAnalyzer::ShowTreeView( bool checked )
{
	//TODO: bad code
	if (checked)
	{
		tbSelections->setChecked( false );
		m_selView->hide();

		m_treeView->setParent( selectionsExplorer );
		QVBoxLayout * layout = new QVBoxLayout();
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing( 0 );
		layout->addWidget( m_treeView );
		delete selectionsExplorer->layout();
		selectionsExplorer->setLayout( layout );
		m_treeView->show();
		selectionsExplorer->show();
	}
	else
	{
		selectionsExplorer->hide();
	}
}

void iAFeatureAnalyzer::selectionLoaded( iASelection * sel )
{
	m_treeView->setSelection( &sel->treeItems );
	m_treeView->loadFilteredDataToOverview();
	m_pdmView->setSelection( sel->pdmSelection );
	loadOverviewSelectionToSPM( m_pdmView->SelectedIndices() );
	//m_spmView->SetData( m_treeView->GetSPMData() );
	m_spmView->setSelection( sel );
}

void iAFeatureAnalyzer::tabChanged( int /*index*/ )
{
	//emit loadTreeDataToViews();
}

bool iAFeatureAnalyzer::doSaveProject(QString const& projectFileName)
{
	iAFeatureAnalyzerTool tool(nullptr, nullptr);
	tool.setOptions(m_dataDir, m_datasetsDir);
	iAProjectFileIO io;
	if (!projectFileName.toLower().endsWith(io.extensions()[0]))
	{
		LOG(lvlError, QString("Only extension %1 is supported!").arg(io.extensions()[0]));
		return false;
	}
	// TODO: Unify with iAMdiChild::doSaveProject
	QSettings projectFile(projectFileName, QSettings::IniFormat);
	projectFile.setValue("UseMdiChild", false);
	projectFile.beginGroup(iAFeatureAnalyzerTool::ID);
	tool.saveState(projectFile, projectFileName);
	projectFile.endGroup();
	return true;
}

//void iAFeatureAnalyzer::GenerateMasksData()
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
//}

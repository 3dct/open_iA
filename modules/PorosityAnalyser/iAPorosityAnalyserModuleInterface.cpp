/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAPorosityAnalyserModuleInterface.h"

#include "iACalculatePoreProperties.h"
#include "iADataFolderDialog.h"
#include "iADatasetInfo.h"
#include "iADragFilterWidget.h"
#include "iADropPipelineWidget.h"
//#include "PorosityAnalyserHelpers.h"
#include "iAPorosityAnalyser.h"
#include "iARunBatchThread.h"

#include <iACPUID.h>
#include <defines.h>
#include <iACSVToQTableWidgetConverter.h>
#include <mainwindow.h>

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QScopedPointer>
#include <QScrollArea>
#include <QSettings>
#include <QTextEdit>
#include <QTime>


const QStringList pbShowLogsText = QStringList() << "Show Logs" << "Hide Logs";
const int minPipelineSlotsCount = 4;
const int maxPipelineSlotsCount = 10;

void iAPorosityAnalyserModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	qsrand(QTime::currentTime().msec());

	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QMenu * menuPorosityAnalyser = getMenuWithTitle( toolsMenu, QString( "FeatureAnalyzer" ), false );

	QAction * actionComputeSegmentations = new QAction( m_mainWnd );
	actionComputeSegmentations->setText( QApplication::translate( "MainWindow", "Compute Segmentations", 0 ) );

	//QAction * actionCalcPoreProps = new QAction( m_mainWnd );
	//actionCalcPoreProps->setText( QApplication::translate( "MainWindow", "Compute Pore Properties", 0 ) );

	QAction * actionRunPA = new QAction( m_mainWnd );
	actionRunPA->setText( QApplication::translate( "MainWindow", "Analyze Segmentations", 0 ) );

	menuPorosityAnalyser->addAction( actionComputeSegmentations );
	//menuPorosityAnalyser->addAction( actionCalcPoreProps );
	menuPorosityAnalyser->addAction( actionRunPA );

	//connect signals to slots
	connect( actionComputeSegmentations, SIGNAL( triggered() ), this, SLOT( computeParameterSpace() ) );
	//connect( actionCalcPoreProps, SIGNAL( triggered() ), this, SLOT( launchCalcPoreProps() ) );
	connect( actionRunPA, SIGNAL( triggered() ), this, SLOT( launchPorosityAnalyser() ) );

	//Read settings
	QSettings settings( organisationName, applicationName );
	m_computerName = settings.value( "FeatureAnalyzer/Computation/computerName", "" ).toString();
	m_resultsFolder = settings.value( "FeatureAnalyzer/Computation/resultsFolder", "" ).toString();
	m_datasetsFolder = settings.value( "FeatureAnalyzer/Computation/datasetsFolder", "" ).toString();
	m_csvFile = settings.value( "FeatureAnalyzer/Computation/csvFile", "" ).toString();

	//Initialize compute segmentation window
	m_compSegmWidget = new QDialog(m_mainWnd);
	m_compSegmWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
	uiComputeSegm.setupUi( m_compSegmWidget );
	uiComputeSegm.Logs->hide();
	uiComputeSegm.csvFilename->setText( m_csvFile );
	uiComputeSegm.computerName->setText( m_computerName );
	uiComputeSegm.resultsFolder->setText( m_resultsFolder );
	uiComputeSegm.datasetsFolder->setText( m_datasetsFolder );
	uiComputeSegm.pBDatasetPreviewProgress->hide();
	connect( uiComputeSegm.computerName, SIGNAL( editingFinished() ), this, SLOT( compNameChanged() ) );
	connect( uiComputeSegm.tbReload, SIGNAL( clicked() ), this, SLOT( loadCSV() ) );
	connect( uiComputeSegm.tbSave, SIGNAL( clicked() ), this, SLOT( saveCSV() ) );
	connect( uiComputeSegm.tbOpenCSV, SIGNAL( clicked() ), this, SLOT( browseCSV() ) );
	connect( uiComputeSegm.tbOpenResultsFolder, SIGNAL( clicked() ), this, SLOT( browserResultsFolder() ) );
	connect( uiComputeSegm.tbOpenDatasetsFolder, SIGNAL( clicked() ), this, SLOT( browserDatasetsFolder() ) );
	connect( uiComputeSegm.pbRunCalculations, SIGNAL( clicked() ), this, SLOT( runCalculations() ) );
	connect( uiComputeSegm.pbShowLogs, SIGNAL( clicked() ), this, SLOT( showHideLogs() ) );
	connect( uiComputeSegm.tbDatasetPreview, SIGNAL( clicked() ), this, SLOT( generateDatasetPreviews() ) );

	//Table widget functionalities
	createTableWidgetActions();
	setupTableWidgetContextMenu();

	//Get CPU info
	m_cpuVendor = iACPUID::cpuVendor().toLocal8Bit().data();
	m_cpuBrand = iACPUID::cpuBrand().toLocal8Bit().data();
}

void iAPorosityAnalyserModuleInterface::computeParameterSpace()
{
	// TODO: move comp segmentation dialog to own class

	while ( QWidget* w = uiComputeSegm.dragWidget->findChild<QWidget*>() )
		delete w;

	m_pipelineSlotIconSize = QPixmap( ":/images/dataset2.png" ).size();
	if ( m_pipelineSlotsCount > 0 )
		m_compSegmWidget->setFixedWidth( m_compSegmWidget->width() -
		( ( m_pipelineSlotsCount - minPipelineSlotsCount ) *
		m_pipelineSlotIconSize.width() ) );
	m_pipelineSlotsCount = minPipelineSlotsCount;

	// Datasets box
	QDir datasetDir( m_datasetsFolder );
	datasetDir.setNameFilters( QStringList() << "*.mhd" );
	QStringList datasetNameList = datasetDir.entryList();
	datasetDir.setNameFilters( QStringList() << "*_GT.mhd" );
	QStringList gtDatasetList = datasetDir.entryList();
	removeGTDatasets( datasetNameList, gtDatasetList );

	QGroupBox *groupBoxDatasets = new QGroupBox( tr( "Datasets" ), uiComputeSegm.dragWidget );
	iADragFilterWidget *dragWidgetDatasets = new iADragFilterWidget( m_datasetsFolder, datasetNameList, 1, groupBoxDatasets );
	dragWidgetDatasets->setObjectName( "dragDatasetWidget" );
	QHBoxLayout *gbDatasetsLayout = new QHBoxLayout;
	dragWidgetDatasets->setFixedSize( m_pipelineSlotIconSize.width(),
									  datasetNameList.size() * ( m_pipelineSlotIconSize.height() + 10 ) );
	QScrollArea *datasetScrollArea = new QScrollArea();
	datasetScrollArea->setStyleSheet( "background-color: white" );
	datasetScrollArea->setWidget( dragWidgetDatasets );
	datasetScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	datasetScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	groupBoxDatasets->setStyleSheet( "QGroupBox::title{ color: gray }" );
	groupBoxDatasets->setFixedWidth( datasetNameList.size() > 6 ? 130 : 85 );
	groupBoxDatasets->setFixedHeight( 530 );
	gbDatasetsLayout->addWidget( datasetScrollArea );
	groupBoxDatasets->setLayout( gbDatasetsLayout );

	// Filters box
	QGroupBox *groupBoxFilters = new QGroupBox( tr( "Filters" ), uiComputeSegm.dragWidget );
	iADragFilterWidget *dragWidgetFilters = new iADragFilterWidget( m_datasetsFolder, datasetNameList, 0, groupBoxFilters );
	dragWidgetFilters->setObjectName( "dragFilterWidget" );
	QHBoxLayout *gbFiltersLayout = new QHBoxLayout;
	QScrollArea *filtersScrollArea = new QScrollArea();
	filtersScrollArea->setStyleSheet( "background-color: white" );
	filtersScrollArea->setWidget( dragWidgetFilters );
	filtersScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	filtersScrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	groupBoxFilters->setStyleSheet( "QGroupBox::title{ color: gray }" );
	groupBoxFilters->setFixedWidth( 440 );
	groupBoxFilters->setFixedHeight( 530 );
	gbFiltersLayout->addWidget( filtersScrollArea );
	groupBoxFilters->setLayout( gbFiltersLayout );

	// Pipeline box
	QGroupBox *m_groupBoxPipeline = new QGroupBox( tr( "Pipeline" ), uiComputeSegm.dragWidget );
	m_groupBoxPipeline->setObjectName( "groupBoxPipeline" );
	m_groupBoxPipeline->setStyleSheet( "QGroupBox::title{ color: gray }" );
	m_groupBoxPipeline->setFixedHeight( 530 );
	m_groupBoxPipeline->setFixedWidth( m_pipelineSlotIconSize.width() * m_pipelineSlotsCount + 30 );

	QWidget *dropHintContainer = new QWidget( m_groupBoxPipeline );
	QHBoxLayout *hLayoutDropHintContainer = new QHBoxLayout;
	hLayoutDropHintContainer->setMargin( 0 );
	hLayoutDropHintContainer->setSpacing( 0 );
	QTextEdit *lHint = new QTextEdit( dropHintContainer );
	lHint->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	lHint->setStyleSheet( "QTextEdit { background-color : #ffffe1; color : black; border: 1px solid black; }" );
	lHint->setHtml( "<p ><b>To set up a segmentation pipeline drag and drop a dataset onto the first pipeline slot. "
					"Do the same for the filters.</b>< / p>"
					"<p>&nbsp;&nbsp;&nbsp;Drag symbol = left mouse press + mouse move"
					"<br>&nbsp;&nbsp;&nbsp;Erase symbol = right mouse click"
					"<br>&nbsp;&nbsp;&nbsp;Modify symbol = CTRL + left mouse click< / p>" );
	lHint->setReadOnly( true );
	//lHint->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	lHint->setFixedHeight( 100 );
	hLayoutDropHintContainer->addWidget( lHint );
	dropHintContainer->setLayout( hLayoutDropHintContainer );

	QWidget *dropModPipelineButtonsContainer = new QWidget( m_groupBoxPipeline );
	QHBoxLayout *hLayoutDropModPipelineButtonsContainer = new QHBoxLayout;
	hLayoutDropModPipelineButtonsContainer->setAlignment( Qt::AlignLeft );
	hLayoutDropModPipelineButtonsContainer->setMargin( 0 );
	QToolButton *btIncPipeline = new QToolButton( dropModPipelineButtonsContainer );
	btIncPipeline->setObjectName( "incPipelineButton" );
	QPixmap incImage( ":/images/add_pipe_slot.png" );
	btIncPipeline->setIcon( QIcon( incImage ) );
	btIncPipeline->setIconSize( QSize( 23, 23 ) );
	btIncPipeline->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	btIncPipeline->setToolTip( "Adds a new slot to the pipeline." );
	hLayoutDropModPipelineButtonsContainer->addWidget( btIncPipeline );
	QToolButton *btDecPipeline = new QToolButton( dropModPipelineButtonsContainer );
	btDecPipeline->setObjectName( "decPipelineButton" );
	QPixmap decImage( ":/images/rem_pipe_slot.png" );
	btDecPipeline->setIcon( QIcon( decImage ) );
	btDecPipeline->setIconSize( QSize( 23, 23 ) );
	btDecPipeline->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	btDecPipeline->setToolTip( "Removes a slot from the pipeline." );
	hLayoutDropModPipelineButtonsContainer->addWidget( btDecPipeline );
	dropModPipelineButtonsContainer->setLayout( hLayoutDropModPipelineButtonsContainer );

	QWidget *m_dropPipelineContainer = new QWidget( m_groupBoxPipeline );
	m_dropPipelineContainer->setObjectName( "dropPipelineContainer" );
	m_dropPipelineContainer->setFixedWidth( m_pipelineSlotIconSize.width() * m_pipelineSlotsCount );
	QHBoxLayout *m_hLayoutDropPipelineContainer = new QHBoxLayout;
	iADropPipelineWidget* m_dropPipelineWidget = new iADropPipelineWidget( m_pipelineSlotIconSize.width() * m_pipelineSlotsCount,
																		   m_pipelineSlotsCount, m_datasetsFolder, m_dropPipelineContainer );
	m_dropPipelineWidget->setObjectName( "dropPipelineWidget" );
	m_hLayoutDropPipelineContainer->addWidget( m_dropPipelineWidget );
	m_hLayoutDropPipelineContainer->setMargin( 0 );
	m_hLayoutDropPipelineContainer->setSpacing( 0 );
	m_dropPipelineContainer->setLayout( m_hLayoutDropPipelineContainer );
	btIncPipeline->setIcon( QIcon( incImage ) );
	btIncPipeline->setIconSize( QSize( 23, 23 ) );
	QWidget *dropAcceptPipelineButtonsContainer = new QWidget( m_groupBoxPipeline );
	QHBoxLayout *hLayoutDropAcceptPipelineButtonsContainer = new QHBoxLayout;
	hLayoutDropAcceptPipelineButtonsContainer->setAlignment( Qt::AlignRight );
	hLayoutDropAcceptPipelineButtonsContainer->setMargin( 0 );
	QToolButton *btClearPipeline = new QToolButton( dropAcceptPipelineButtonsContainer );
	QPixmap remImage( ":/images/rem_pipe.png" );
	btClearPipeline->setIcon( QIcon( remImage ) );
	btClearPipeline->setIconSize( QSize( 23, 23 ) );
	btClearPipeline->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	btClearPipeline->setToolTip( "Deletes the entire pipeline." );
	hLayoutDropAcceptPipelineButtonsContainer->addWidget( btClearPipeline );
	QToolButton *btAddPipeline = new QToolButton( dropAcceptPipelineButtonsContainer );
	QPixmap accImage( ":/images/acc_pipe.png" );
	btAddPipeline->setIcon( QIcon( accImage ) );
	btAddPipeline->setIconSize( QSize( 23, 23 ) );
	btAddPipeline->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	btAddPipeline->setToolTip( "Adds the pipeline to the table below." );
	hLayoutDropAcceptPipelineButtonsContainer->addWidget( btAddPipeline );
	dropAcceptPipelineButtonsContainer->setLayout( hLayoutDropAcceptPipelineButtonsContainer );

	QWidget* vSpacer1 = new QWidget();
	vSpacer1->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	vSpacer1->setFixedHeight( 1 );
	QWidget* vSpacer2 = new QWidget();
	vSpacer2->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	QVBoxLayout *gbPipelineLayout = new QVBoxLayout;
	gbPipelineLayout->addWidget( dropHintContainer );
	gbPipelineLayout->addWidget( vSpacer1 );
	gbPipelineLayout->addWidget( dropModPipelineButtonsContainer );
	gbPipelineLayout->addWidget( m_dropPipelineContainer );
	gbPipelineLayout->addWidget( dropAcceptPipelineButtonsContainer );
	gbPipelineLayout->addWidget( vSpacer2 );
	m_groupBoxPipeline->setLayout( gbPipelineLayout );

	uiComputeSegm.dragWidget->layout()->addWidget( groupBoxDatasets );
	uiComputeSegm.dragWidget->layout()->addWidget( groupBoxFilters );
	uiComputeSegm.dragWidget->layout()->addWidget( m_groupBoxPipeline );

	connect( btClearPipeline, SIGNAL( clicked() ), this, SLOT( clearPipeline() ) );
	connect( btAddPipeline, SIGNAL( clicked() ), this, SLOT( addPipeline() ) );
	connect( btIncPipeline, SIGNAL( clicked() ), this, SLOT( resizePipeline() ) );
	connect( btDecPipeline, SIGNAL( clicked() ), this, SLOT( resizePipeline() ) );

	// Set tooltip style of the table buttons
	uiComputeSegm.tbRemoveRow->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	uiComputeSegm.tbSaveTable->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );
	uiComputeSegm.tbLoadTable->setStyleSheet( "QToolTip { color: black; background-color: #ffffe1; border: 1px solid black; }" );

	m_compSegmWidget->show();
	loadCSV();
}

void iAPorosityAnalyserModuleInterface::removeGTDatasets( QStringList& list, const QStringList& toDelete ){
	QStringListIterator i( toDelete );
	while ( i.hasNext() )
		list.removeAll( i.next() );
}

void iAPorosityAnalyserModuleInterface::loadCSV()
{
	iACSVToQTableWidgetConverter::loadCSVFile( uiComputeSegm.csvFilename->text(), uiComputeSegm.tableWidget );
	uiComputeSegm.tableWidget->resizeColumnsToContents();
}

void iAPorosityAnalyserModuleInterface::saveCSV()
{
	iACSVToQTableWidgetConverter::saveToCSVFile( *uiComputeSegm.tableWidget, uiComputeSegm.csvFilename->text() );
}

void iAPorosityAnalyserModuleInterface::browseCSV()
{
	QString csvFile = QFileDialog::getOpenFileName( m_mainWnd, tr( "Computation Parameters File (CSV)" ),
		m_mainWnd->path(), tr( "CSV Files (*.csv *.CSV)" ) );
	uiComputeSegm.csvFilename->setText( csvFile );
	m_csvFile = csvFile;
	loadCSV();
}

void iAPorosityAnalyserModuleInterface::displayPipelineInSlots( QTableWidgetItem * item)
{
	if ( QGuiApplication::queryKeyboardModifiers().testFlag( Qt::AltModifier ) )
	{
		QTableWidget* tw = qobject_cast<QTableWidget*>( sender() );
		int selRow = tw->row( item );
		if ( selRow ) // not header
		{
			QStringList algoList = tw->item( selRow, 0 )->text().split( "_" );
			QList<QLabel*> algoLabelList;
			iADragFilterWidget* dfw = uiComputeSegm.dragWidget->findChild<iADragFilterWidget*>( "dragFilterWidget" );
			for ( int i = 0; i < algoList.length(); ++i )
				algoLabelList.append( dfw->getLabel( algoList[i] ) );

			//iADragFilterWidget* ddw = uiComputeSegm.dragWidget->findChild<iADragFilterWidget*>( "dragDatasetWidget" );
			//QString dataset = QString( "dataset_" + tw->item( selRow, 1 )->text() );
			//QLabel* datsetLabel = ddw->getLabel( dataset );
		}
	}
}

void iAPorosityAnalyserModuleInterface::SaveSettings() const
{
	updateFromGUI();
	QSettings settings( organisationName, applicationName );
	settings.setValue( "FeatureAnalyzer/Computation/computerName", m_computerName );
	settings.setValue( "FeatureAnalyzer/Computation/resultsFolder", m_resultsFolder );
	settings.setValue( "FeatureAnalyzer/Computation/datasetsFolder", m_datasetsFolder );
	settings.setValue( "FeatureAnalyzer/Computation/csvFile", m_csvFile );
}

void iAPorosityAnalyserModuleInterface::updateFromGUI() const
{
	m_computerName = uiComputeSegm.computerName->text();
	m_resultsFolder = uiComputeSegm.resultsFolder->text();
	m_datasetsFolder = uiComputeSegm.datasetsFolder->text();
	m_csvFile = uiComputeSegm.csvFilename->text();
}

void iAPorosityAnalyserModuleInterface::browserResultsFolder()
{
	m_resultsFolder = QFileDialog::getExistingDirectory( m_mainWnd, tr( "Results folder" ), m_resultsFolder,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	uiComputeSegm.resultsFolder->setText( m_resultsFolder );
}

void iAPorosityAnalyserModuleInterface::browserDatasetsFolder()
{
	m_datasetsFolder = QFileDialog::getExistingDirectory( m_mainWnd, tr( "Datasets folder" ), m_datasetsFolder,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
	uiComputeSegm.datasetsFolder->setText( m_datasetsFolder );
	computeParameterSpace();	//bad code; creates everything new in uiComputeSegm.dragWidget
}

void iAPorosityAnalyserModuleInterface::runCalculations()
{
	iARunBatchThread * rbt = new iARunBatchThread( this );
	connect( rbt, SIGNAL( batchProgress( int ) ), this, SLOT( batchProgress( int ) ) );
	connect( rbt, SIGNAL( totalProgress( int ) ), this, SLOT( totalProgress( int ) ) );
	connect( rbt, SIGNAL( currentBatch( QString ) ), this, SLOT( currentBatch( QString ) ) );
	rbt->Init(this,
		m_datasetsFolder,
		uiComputeSegm.rbNewPipelineDataNoPores->isChecked(),
		uiComputeSegm.rbNewPipelineData->isChecked());
	rbt->start();
}

void iAPorosityAnalyserModuleInterface::showHideLogs()
{
	if( uiComputeSegm.Logs->isHidden() )
	{
		uiComputeSegm.Logs->show();
		uiComputeSegm.pbShowLogs->setText( pbShowLogsText[1] );
	}
	else
	{
		uiComputeSegm.Logs->hide();
		uiComputeSegm.pbShowLogs->setText( pbShowLogsText[0] );
	}
}

void iAPorosityAnalyserModuleInterface::log( QString text, bool appendToPrev )
{
	if( appendToPrev )
	{
		QString prev_text = uiComputeSegm.Logs->item( uiComputeSegm.Logs->count() - 1 )->text();
		uiComputeSegm.Logs->item( uiComputeSegm.Logs->count() - 1 )->setText( prev_text + text );
	}
	else
		uiComputeSegm.Logs->insertItem( uiComputeSegm.Logs->count(), text );
}

Ui::ComputeSegmentations * iAPorosityAnalyserModuleInterface::ui()
{
	return &uiComputeSegm;
}

QString iAPorosityAnalyserModuleInterface::DatasetFolder() const
{
	return m_datasetsFolder;
}

QString iAPorosityAnalyserModuleInterface::ResultsFolder() const
{
	return m_resultsFolder;
}

QString iAPorosityAnalyserModuleInterface::CSVFile() const
{
	return m_csvFile;
}

void iAPorosityAnalyserModuleInterface::batchProgress( int progress )
{
	uiComputeSegm.batchProgress->setValue( progress );
	QCoreApplication::processEvents();
}

void iAPorosityAnalyserModuleInterface::totalProgress( int progress )
{
	uiComputeSegm.totalProgress->setValue( progress );
	QCoreApplication::processEvents();
}

void iAPorosityAnalyserModuleInterface::currentBatch(QString str )
{
	uiComputeSegm.laBatchProgress->setText( str );
	QCoreApplication::processEvents();
}

QString iAPorosityAnalyserModuleInterface::ComputerName() const
{
	return m_computerName;
}

void iAPorosityAnalyserModuleInterface::launchPorosityAnalyser()
{
	iADataFolderDialog * dlg = new iADataFolderDialog( m_mainWnd );
	if( !dlg->exec() == QDialog::Accepted )
		return;

	m_porosityAnalyser = new iAPorosityAnalyser(m_mainWnd, dlg->ResultsFolderName(), dlg->DatasetsFolderName(), m_mainWnd );
	m_mainWnd->addSubWindow( m_porosityAnalyser );
	m_porosityAnalyser->LoadStateAndShow(); //show();
}

//void iAPorosityAnalyserModuleInterface::launchCalcPoreProps()
//{
//	m_calcPoreProps = new iACalculatePorePropeties( );
//	m_mainWnd->addSubWindow( m_calcPoreProps );
//	m_calcPoreProps->show();
//}

void iAPorosityAnalyserModuleInterface::clearPipeline()
{
	iADropPipelineWidget* dpw = uiComputeSegm.dragWidget->findChild<iADropPipelineWidget*>( "dropPipelineWidget" );
	dpw->clearAllSlots();
}

void iAPorosityAnalyserModuleInterface::addPipeline()
{
	iADropPipelineWidget* dpw = uiComputeSegm.dragWidget->findChild<iADropPipelineWidget*>( "dropPipelineWidget" );
	QList<QList<QString>> pipeline = dpw->getPipeline();

	if ( !pipeline.size() )
		return;

	int lastRow = uiComputeSegm.tableWidget->rowCount(), totalPipeParts = 0, totalParams = 0;

	for ( int part = 0; part < pipeline.size(); ++part )
	{
		if ( pipeline[part].at( 0 ).length() > 0 )
		{
			totalPipeParts++;
			totalParams += ( pipeline[part].size() - 2 ) / 3;
		}
	}

	// Empty slots at the end of the pipeline are allowed (gaps not)
	for ( int j = 0; j < totalPipeParts; ++j )
	{
		if ( pipeline[j].at( 0 ).length() == 0 )
		{
			QMessageBox msgBox;
			msgBox.setText( "The pipeline is not connected. Please close the gap by rearranging the filters.  " );
			msgBox.setWindowTitle( "FeatureAnalyzer" );
			msgBox.exec();
			return;
		}
	}

	QString algoStr, datasetStr = "dataset is missing", randSamplStr = "0";

	// Find datasetName in the pipeline list
	int datasetNameIdx = -1;
	for ( int i = 0; i < pipeline.size(); ++i )
	{
		if ( pipeline[i][0].left( 8 ) == "dataset_" )
			datasetNameIdx = i;
	}

	// No parameter setting without a dataset
	if ( datasetNameIdx == -1 )
	{
		QMessageBox msgBox;
		msgBox.setText( "No dataset. First pipeline position must be a dataset." );
		msgBox.setWindowTitle( "FeatureAnalyzer" );
		msgBox.exec();
		return;
	}

	// Always move dataset item to pipeline pos 0
	pipeline.move( datasetNameIdx, 0 );

	datasetStr = pipeline[0][0];
	datasetStr.remove( 0, 8 );

	for ( int i = 1; i < totalPipeParts; ++i )
	{
		// Chain filters
		algoStr.append( pipeline.at( i ).at( 0 ) );
		if ( i != totalPipeParts - 1 )
			algoStr.append( "_" );

		// Random sampling must be set
		if ( pipeline.at( i ).size() < 2 )
		{
			QMessageBox msgBox;
			msgBox.setText( "Some parameters are missing. Modify parameters by double clicking on a pipeline icon " );
			msgBox.setWindowTitle( "FeatureAnalyzer" );
			msgBox.exec();
			return;
		}

		// Aggregate random sampling
		if ( pipeline.at( i ).at( 1 ) == "$Random Sampling 1" )
			randSamplStr = "1";
	}

	if ( !algoStr.size() )
	{
		QMessageBox msgBox;
		msgBox.setText( "Pipeline is not complete. There is only a dataset." );
		msgBox.setWindowTitle( "FeatureAnalyzer" );
		msgBox.exec();
		return;
	}

	int currCol = 0;
	uiComputeSegm.tableWidget->setRowCount( lastRow + 1 );
	uiComputeSegm.tableWidget->setItem( lastRow, currCol++, new QTableWidgetItem( algoStr ) );
	uiComputeSegm.tableWidget->setItem( lastRow, currCol++, new QTableWidgetItem( datasetStr ) );
	uiComputeSegm.tableWidget->setItem( lastRow, currCol++, new QTableWidgetItem( randSamplStr ) );

	if ( uiComputeSegm.tableWidget->columnCount() < totalParams + currCol )
		uiComputeSegm.tableWidget->setColumnCount( totalParams + currCol );

	for ( int i = 0; i < totalPipeParts; ++i )
	{
		QList<QString> paramList;
		QString filter = pipeline.at( i ).at( 0 );
		// Check non-parametric filter
		if ( filterNames.indexOf( filter ) != 4 &&
			 !( filterNames.indexOf( filter ) < 16 && filterNames.indexOf( filter ) > 7 ) )
		{
			// Altering column entries
			for ( int j = 2; j < pipeline.at( i ).size(); )
			{
				QString paramStr;
				paramStr.append( pipeline.at( i ).at( j++ ).section( ' ', 2, 2 ) + " " );
				paramStr.append( pipeline.at( i ).at( j++ ).section( ' ', 2, 2 ) + " " );
				paramStr.append( pipeline.at( i ).at( j++ ).section( ' ', 2, 2 ) );
				paramList.append( paramStr );
			}

			for ( int k = 0; k < paramList.size(); ++k )
			{
				//TODO: maybe setcolumCount
				uiComputeSegm.tableWidget->setItem( lastRow, currCol++, new QTableWidgetItem( paramList.at( k ) ) );
			}
		}
	}
	uiComputeSegm.tableWidget->scrollToBottom();
	iACSVToQTableWidgetConverter::saveToCSVFile( *uiComputeSegm.tableWidget, uiComputeSegm.csvFilename->text() );
}

void iAPorosityAnalyserModuleInterface::resizePipeline()
{
	QToolButton* tb = qobject_cast<QToolButton*>( sender() );
	iADropPipelineWidget* dpw = uiComputeSegm.dragWidget->findChild<iADropPipelineWidget*>( "dropPipelineWidget" );
	if ( tb->objectName() == "incPipelineButton" )
	{
		if ( m_pipelineSlotsCount < maxPipelineSlotsCount )
		{
			m_pipelineSlotsCount++;
			m_compSegmWidget->setFixedWidth( m_compSegmWidget->width() + m_pipelineSlotIconSize.width() );
		}
	}

	if ( tb->objectName() == "decPipelineButton" )
	{
		if ( !dpw->isLastPipelineSlotEmpty() )
		{
			QMessageBox msgBox;
			msgBox.setText( "Slot cannot be removed. There is still a filter in the last slot." );
			msgBox.setWindowTitle( "FeatureAnalyzer" );
			msgBox.exec();
			return;
		}

		if ( m_pipelineSlotsCount > minPipelineSlotsCount )
		{
			m_pipelineSlotsCount--;
			m_compSegmWidget->setFixedWidth( m_compSegmWidget->width() - m_pipelineSlotIconSize.width() );
		}
		else
		{
			QMessageBox msgBox;
			msgBox.setText( "Cannot decrease pipeline size." );
			msgBox.setWindowTitle( "FeatureAnalyzer" );
			msgBox.exec();
			return;
		}
	}

	dpw->updatePipelineSlots( m_pipelineSlotsCount, m_pipelineSlotIconSize.width() );
	QWidget* dpc = uiComputeSegm.dragWidget->findChild<QWidget*>( "dropPipelineContainer" );
	dpc->setFixedWidth( m_pipelineSlotsCount * m_pipelineSlotIconSize.width() );
	QWidget* gbp = uiComputeSegm.dragWidget->findChild<QWidget*>( "groupBoxPipeline" );
	gbp->setFixedWidth( m_pipelineSlotsCount * m_pipelineSlotIconSize.width() + 30 );
}

void iAPorosityAnalyserModuleInterface::clearTableWidgetItem()
{
	int currRow = uiComputeSegm.tableWidget->currentRow();
	if ( currRow > 0 )
	{
		uiComputeSegm.tableWidget->removeRow( currRow );
		saveCSV();
	}
}

void iAPorosityAnalyserModuleInterface::setupTableWidgetContextMenu()
{
	uiComputeSegm.tableWidget->addAction( removeRowAction );
	uiComputeSegm.tableWidget->addAction( saveTableToCSVAction );
	uiComputeSegm.tableWidget->addAction( loadTableFromCSVAction );
	uiComputeSegm.tableWidget->setContextMenuPolicy( Qt::ActionsContextMenu );
	uiComputeSegm.tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
}

void iAPorosityAnalyserModuleInterface::createTableWidgetActions()
{
	removeRowAction = new QAction( tr( "Remove Row" ), this );
	removeRowAction->setShortcut( Qt::Key_Delete );
	saveTableToCSVAction = new QAction( tr( "Save Table" ), this );
	saveTableToCSVAction->setShortcut( Qt::CTRL | Qt::Key_S );
	loadTableFromCSVAction = new QAction( tr( "Load Table" ), this );
	loadTableFromCSVAction->setShortcut( Qt::CTRL | Qt::Key_L );

	connect( removeRowAction, SIGNAL( triggered() ), this, SLOT( clearTableWidgetItem() ) );
	connect( uiComputeSegm.tbRemoveRow, SIGNAL( clicked() ), this, SLOT( clearTableWidgetItem() ) );
	connect( saveTableToCSVAction, SIGNAL( triggered() ), this, SLOT( saveCSV() ) );
	connect( uiComputeSegm.tbSaveTable, SIGNAL( clicked() ), this, SLOT( saveCSV() ) );
	connect( loadTableFromCSVAction, SIGNAL( triggered() ), this, SLOT( loadCSV() ) );
	connect( uiComputeSegm.tbLoadTable, SIGNAL( clicked() ), this, SLOT( loadCSV() ) );
	connect( uiComputeSegm.tableWidget, SIGNAL( itemClicked( QTableWidgetItem * ) ),
			 this, SLOT( displayPipelineInSlots( QTableWidgetItem * ) ) );
}

void iAPorosityAnalyserModuleInterface::generateDatasetPreviews()
{
	iADatasetInfo * prev = new iADatasetInfo( this );
	connect( prev, SIGNAL( finished() ), this, SLOT( datasetPreviewThreadFinished() ) );
	connect( prev, SIGNAL( started() ), this, SLOT( datasetPreviewThreadStarted() ) );
	connect( prev, SIGNAL( progress( int ) ), uiComputeSegm.pBDatasetPreviewProgress, SLOT( setValue( int ) ) );
	uiComputeSegm.pBDatasetPreviewProgress->setValue( 0 );
	prev->start();
}

void iAPorosityAnalyserModuleInterface::datasetPreviewThreadFinished()
{
	iADatasetInfo * di = qobject_cast<iADatasetInfo*>( sender() );
	iADragFilterWidget * ddw = uiComputeSegm.dragWidget->findChild<iADragFilterWidget*>( "dragDatasetWidget" );
	QStringList datasetList = di->getNewGeneratedInfoFiles().replaceInStrings( ".info", "" );
	QStringList GTDatasetsList = datasetList.filter( "_GT.mhd" );
	removeGTDatasets( datasetList, GTDatasetsList );
	ddw->updateDatasetTooltip( datasetList );
	uiComputeSegm.pBDatasetPreviewProgress->hide();
}

void iAPorosityAnalyserModuleInterface::datasetPreviewThreadStarted()
{
	uiComputeSegm.pBDatasetPreviewProgress->show();
}

void iAPorosityAnalyserModuleInterface::compNameChanged()
{
	m_computerName = uiComputeSegm.computerName->text();;
}
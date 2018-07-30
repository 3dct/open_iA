/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAFeatureScoutModuleInterface.h"

#include "dlg_CSVInput.h"
#include "iAFeatureScoutAttachment.h"
#include "iAFeatureScoutToolbar.h"
#include "ui_CsvInput.h"

#include "iAConsole.h"
#include "io/iACsvIO.h"
#include "mainwindow.h"

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * FeatureScoutCsvReader = getMenuWithTitle(toolsMenu, QString("FeatureScout"), false);

	//1 Menu mit mehreren untereinträgen

	//adds an entry to feature scout with name featurescout
	QAction * actionFibreScout = new QAction( m_mainWnd );
	actionFibreScout->setText( QApplication::translate( "MainWindow", "FeatureScout", 0 ) );
	AddActionToMenuAlphabeticallySorted(FeatureScoutCsvReader, actionFibreScout);

	connect(actionFibreScout, SIGNAL(triggered()), this, SLOT(FeatureScout()));


	//new entry FeaturescoutWithCSV
	QAction * actionOpenCSVFeatureScout = new QAction(m_mainWnd);
	actionOpenCSVFeatureScout->setText(QApplication::translate("MainWindow", "FeatureScoutWithCSV", 0));
	AddActionToMenuAlphabeticallySorted(FeatureScoutCsvReader, actionOpenCSVFeatureScout, false);

	//action mit module verbinden
	connect(actionOpenCSVFeatureScout, &QAction::triggered, this, &iAFeatureScoutModuleInterface::FeatureScoutWithCSV);


	tlbFeatureScout = 0;

}

void iAFeatureScoutModuleInterface::FeatureScoutWithCSV()
{

	csvConfig::configPararams fileConfParams;
	//TODO set file path
	dlg_CSVInput dlg;

	if (dlg.exec() != QDialog::Accepted) {
		return;
	}


	this->m_mdiChild = m_mainWnd->createMdiChild(false);
	this->m_mdiChild->show();

	if (!m_mdiChild) return;
	QVector<uint> selEntriesId;
	QSharedPointer<QStringList> headers = QSharedPointer<QStringList>(new QStringList);
	QSharedPointer<QStringList> featScout_headers = QSharedPointer<QStringList>(new QStringList);

	ulong table_width;
	fileConfParams = dlg.getConfigParameters();
	selEntriesId = dlg.getEntriesSelInd();
	headers = dlg.getHeaderSelection();


	table_width = dlg.getTableWidth();

	QMap<QString, iAObjectAnalysisType> objectMap;
	objectMap["Fibers"] = INDIVIDUAL_FIBRE_VISUALIZATION;
	objectMap["Voids"] = INDIVIDUAL_PORE_VISUALIZATION;

	QStringList items;
	items << tr("Fibers") << tr("Voids");
	QString filterName = tr("FeatureScout"), item;
	if (fileConfParams.inputObjectType == csvConfig::CTInputObjectType::Voids) {
		item = "Voids";
		featScout_headers = headers;
	}
	else {
		item = "Fibers";
		*featScout_headers = io.GetFibreElementsName(true);
		//headers = dlg.getAllHeaders();

	}

	io.setParams(*headers, selEntriesId, table_width);
	//featScout_headers = headers;


	if (!fileConfParams.fileName.isEmpty()) {
		initializeFeatureScoutStartUp(item, items, fileConfParams.fileName, objectMap, filterName, true, &fileConfParams, featScout_headers /*headers*/);
	}
	else m_mdiChild->addMsg("CSV-file name error.");



};



void iAFeatureScoutModuleInterface::FeatureScout()
{
	PrepareActiveChild();
	QString fileName = QFileDialog::getOpenFileName(m_mdiChild, tr("Select CSV File"), m_mdiChild->getFilePath(), tr("CSV Files (*.csv)"));
	LoadFeatureScoutWithParams(fileName, m_mdiChild);
}

//optional parameter csvFile name
void iAFeatureScoutModuleInterface::LoadFeatureScoutWithParams(const QString &fileName, MdiChild *mchildWnd)
{
	QMap<QString, iAObjectAnalysisType> objectMap;
	objectMap["Fibers"] = INDIVIDUAL_FIBRE_VISUALIZATION;
	objectMap["Voids"] = INDIVIDUAL_PORE_VISUALIZATION;

	QStringList items;
	items << tr( "Fibers" ) << tr( "Voids" );
	QString filterName = tr( "FeatureScout" ), item;
	m_mdiChild = mchildWnd;

	if ( !fileName.isEmpty() )
	{
		QFile file( fileName );
		if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
		{
			// TODO: create convention, 2nd line of csv file for fibers (pore csv file have this line)
			// Automatic csv file detection
			QTextStream in( &file );
			in.readLine();
			item = in.readLine();
			if ( item != "Voids" )
				item = "Fibers";
			file.close();

			initializeFeatureScoutStartUp(item, items, fileName, objectMap, filterName, false, nullptr, QSharedPointer<QStringList>());
		}
		else
			m_mdiChild->addMsg( "CSV-file could not be opened." );
	}
	else
		m_mdiChild->addMsg( "CSV-file name error." );
}

void iAFeatureScoutModuleInterface::initializeFeatureScoutStartUp(QString &item, QStringList &items, QString const &fileName, QMap<QString,
	iAObjectAnalysisType> &objectMap, QString const &filterName, const bool isCsvOnly, csvConfig::configPararams *FileParams, const QSharedPointer<QStringList> &selHeaders)
{
	if (item == items[0] || item == items[1])
	{
		if (m_mdiChild && filter_FeatureScout(m_mdiChild, fileName, objectMap[item], FileParams, isCsvOnly, selHeaders))
		{
			SetupToolbar();
			m_mdiChild->addStatusMsg(filterName);
			setFeatureScoutRenderSettings();
			m_mdiChild->addMsg("The render settings of the current mdiChild"
				" window have been adapted to the FeatureScout!");
		}
	}
	else
		m_mdiChild->addMsg("CSV-file header error.");
}

void iAFeatureScoutModuleInterface::SetupToolbar()
{
	if ( tlbFeatureScout )
	{
		return;
	}
	tlbFeatureScout = new iAFeatureScoutToolbar( m_mainWnd );
	m_mainWnd->addToolBar( Qt::BottomToolBarArea, tlbFeatureScout );
	connect( tlbFeatureScout->actionLength_Distribution, SIGNAL( triggered() ), this, SLOT( FeatureScout_Options() ) );
	connect( tlbFeatureScout->actionMeanObject, SIGNAL( triggered() ), this, SLOT( FeatureScout_Options() ) );
	connect( tlbFeatureScout->actionMultiRendering, SIGNAL( triggered() ), this, SLOT( FeatureScout_Options() ) );
	connect( tlbFeatureScout->actionOrientation_Rendering, SIGNAL( triggered() ), this, SLOT( FeatureScout_Options() ) );
	connect( tlbFeatureScout->actionActivate_SPM, SIGNAL( triggered() ), this, SLOT( FeatureScout_Options() ) );
	tlbFeatureScout->setVisible( true );
}

void iAFeatureScoutModuleInterface::setFeatureScoutRenderSettings()
{
	iARenderSettings FS_RenderSettings = m_mdiChild->GetRenderSettings();
	iAVolumeSettings FS_VolumeSettings = m_mdiChild->GetVolumeSettings();
	FS_RenderSettings.ParallelProjection = true;
	FS_RenderSettings.ShowHelpers = true;
	FS_RenderSettings.ShowRPosition = true;
	FS_RenderSettings.ShowSlicers = true;
	FS_VolumeSettings.LinearInterpolation = false;
	FS_VolumeSettings.DiffuseLighting = 1.6;
	FS_VolumeSettings.Shading = true;
	FS_VolumeSettings.SpecularLighting = 0.0;
	FS_VolumeSettings.RenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
	m_mdiChild->editRendererSettings(FS_RenderSettings, FS_VolumeSettings);
}

/*entry point for openIA FeatureScout
*optional parameter FileParams for custom csv
*/
bool iAFeatureScoutModuleInterface::filter_FeatureScout( MdiChild* mdiChild, QString fileName, iAObjectAnalysisType objectType, csvConfig::configPararams *FileParams, const bool is_csvOnly, const QSharedPointer<QStringList> &selHeader)
	{
	//default action if file params is null
	if (!FileParams) {

		if (!io.LoadCsvFile(objectType, fileName)) //hier wird das csv geladen;
			return false;
	}
	else {
		if (!io.loadCSVCustom(*FileParams)) {
			return false;
		}

	}

	//enables debug writing out table to desktop
	//io.debugTable(false);

	QString filtername = tr( "FeatureScout started" );
	m_mdiChild->addStatusMsg( filtername );
	m_mdiChild->addMsg( filtername );
	AttachToMdiChild( m_mdiChild );
	connect( m_mdiChild, SIGNAL( closed() ), this, SLOT( onChildClose() ) );
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if ( !attach )
	{
		m_mdiChild->addMsg( "Error while creating FeatureScout module!" );
		return false;
	}


	attach->init(objectType, io.GetCSVTable(), is_csvOnly, selHeader);
	return true;
}

void iAFeatureScoutModuleInterface::FeatureScout_Options()
{
	QAction *action = (QAction *) sender();
	QString actionText = action->text();

	int idx = 0;

	if ( actionText.toStdString() == "Length Distribution" ) idx = 7;
	if ( actionText.toStdString() == "Mean Object" ) idx = 4;
	if ( actionText.toStdString() == "Multi Rendering" ) idx = 3;
	if ( actionText.toStdString() == "Orientation Rendering" ) idx = 5;
	if ( actionText.toStdString() == "Activate SPM" ) idx = 6;

	m_mdiChild = m_mainWnd->activeMdiChild();
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if ( !attach )
	{
		DEBUG_LOG( "No FeatureScout attachment in current MdiChild!" );
		return;
	}
	attach->FeatureScout_Options( idx );

	m_mainWnd->statusBar()->showMessage( tr( "FeatureScout options changed to: " ).append( actionText ), 5000 );
}

void iAFeatureScoutModuleInterface::onChildClose()
{
	m_mainWnd->removeToolBar( tlbFeatureScout );
	delete tlbFeatureScout;
	tlbFeatureScout = 0;
}


iAModuleAttachmentToChild * iAFeatureScoutModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return new iAFeatureScoutAttachment( mainWnd, childData );
}

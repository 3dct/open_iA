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
#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"
#include "iAFeatureScoutAttachment.h"
#include "iAFeatureScoutToolbar.h"
#include "iAModalityList.h"
#include "ui_CsvInput.h"

#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextStream>

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * FeatureScoutCsvReader = getMenuWithTitle(toolsMenu, QString("FeatureScout"), false);

	QAction * actionFibreScout = new QAction( m_mainWnd );
	actionFibreScout->setText( QApplication::translate( "MainWindow", "FeatureScout", 0 ) );
	AddActionToMenuAlphabeticallySorted(FeatureScoutCsvReader, actionFibreScout);
	connect(actionFibreScout, SIGNAL(triggered()), this, SLOT(FeatureScout()));

	QAction * actionOpenCSVFeatureScout = new QAction(m_mainWnd);
	actionOpenCSVFeatureScout->setText(QApplication::translate("MainWindow", "FeatureScoutWithCSV", 0));
	AddActionToMenuAlphabeticallySorted(FeatureScoutCsvReader, actionOpenCSVFeatureScout, false);
	connect(actionOpenCSVFeatureScout, &QAction::triggered, this, &iAFeatureScoutModuleInterface::FeatureScoutWithCSV);

	tlbFeatureScout = nullptr;
}

void iAFeatureScoutModuleInterface::FeatureScoutWithCSV()
{
	dlg_CSVInput dlg;
	if (m_mainWnd->activeMdiChild())
		dlg.setPath(m_mainWnd->activeMdiChild()->getFilePath());
	if (dlg.exec() != QDialog::Accepted)
		return;
	iACsvConfig csvConfig = dlg.getConfig();
	if (csvConfig.useVolumeData && (!m_mainWnd->activeMdiChild() ||
		m_mainWnd->activeMdiChild()->GetModalities()->size() == 0 ||
		!m_mainWnd->activeMdiChild()->IsVolumeDataLoaded()))
	{
		QMessageBox::information(m_mainWnd, "FeatureScout", "You have selected to use volume data in FeatureScout, "
			"yet there is either no open window or the active window does not contain volume data!");
		return;
	}
	if (!csvConfig.useVolumeData)
	{
		m_mdiChild = m_mainWnd->createMdiChild(false);
		this->m_mdiChild->show();
	}
	else
		m_mdiChild = m_mainWnd->activeMdiChild();
	startFeatureScout(csvConfig);
}


void iAFeatureScoutModuleInterface::FeatureScout()
{
	PrepareActiveChild();
	QString fileName = QFileDialog::getOpenFileName( m_mdiChild, tr( "Select CSV File" ),
		m_mdiChild->getFilePath(), tr( "CSV Files (*.csv)" ) );
	LoadFeatureScoutWithParams(fileName, m_mdiChild);
}

void iAFeatureScoutModuleInterface::LoadFeatureScoutWithParams(const QString &csvFileName, MdiChild *mchildWnd)
{
	if ( csvFileName.isEmpty() )
		return;
	m_mdiChild = mchildWnd;
	QFile file( csvFileName );
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		m_mdiChild->addMsg("CSV-file could not be opened.");
		return;
	}
	// TODO: create convention, 2nd line of csv file for fibers (pore csv file have this line)
	// Automatic csv file detection
	QTextStream in( &file );
	in.readLine();
	QString item = in.readLine();
	iACsvConfig csvConfig;
	if (item != "Voids")
		csvConfig = iACsvConfig::getLegacyFiberFormat( csvFileName );
	else
		csvConfig = iACsvConfig::getLegacyPoreFormat( csvFileName );
	file.close();
	startFeatureScout(csvConfig);
}

void iAFeatureScoutModuleInterface::SetupToolbar()
{
	if ( tlbFeatureScout )
		return;
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

void iAFeatureScoutModuleInterface::startFeatureScout(iACsvConfig const & csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
		return;
	AttachToMdiChild( m_mdiChild );
	connect( m_mdiChild, SIGNAL( closed() ), this, SLOT( onChildClose() ) );
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if ( !attach )
	{
		m_mdiChild->addMsg( "Error while attaching FeatureScout to mdi child window!" );
		return;
	}
	attach->init(csvConfig.objectType, csvConfig.fileName, creator.getTable(), !csvConfig.useVolumeData, io.getOutputMapping());
	SetupToolbar();
	m_mdiChild->addStatusMsg("FeatureScout started");
	m_mdiChild->addMsg("FeatureScout started");
	setFeatureScoutRenderSettings();
	m_mdiChild->addMsg("The render settings of the current mdiChild window have been adapted to the FeatureScout!");
	return;
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
	// TODO: check if a second mdi child has FeatureScout open?
	m_mainWnd->removeToolBar( tlbFeatureScout );
	delete tlbFeatureScout;
	tlbFeatureScout = nullptr;
}

iAModuleAttachmentToChild * iAFeatureScoutModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return new iAFeatureScoutAttachment( mainWnd, childData );
}

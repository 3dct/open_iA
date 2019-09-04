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
#include "iAFeatureScoutModuleInterface.h"

#include "dlg_CSVInput.h"
#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"
#include "iAFeatureScoutAttachment.h"
#include "iAFeatureScoutToolbar.h"
#include "iAModalityList.h"
#include "ui_CsvInput.h"

#include <iAConsole.h>
#include <iAProjectBase.h>
#include <iAProjectRegistry.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTextStream>

class iAFeatureScoutProject: public iAProjectBase
{
	virtual ~iAFeatureScoutProject() {}
	void loadProject(QSettings const & projectFile) override;
	void saveProject(QSettings const & projectFile) override;
};

void iAFeatureScoutProject::loadProject(QSettings const & projectFile)
{

}

void iAFeatureScoutProject::saveProject(QSettings const & projectFile)
{

}

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	iAProjectRegistry::addProject<iAFeatureScoutProject>("FeatureScout");
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QAction * actionFibreScout = new QAction( QObject::tr("FeatureScout"), nullptr );
	AddActionToMenuAlphabeticallySorted( toolsMenu, actionFibreScout, false );
	connect(actionFibreScout, SIGNAL(triggered()), this, SLOT(FeatureScout()));
	tlbFeatureScout = nullptr;
}

void iAFeatureScoutModuleInterface::FeatureScout()
{
	bool volumeDataAvailable = m_mainWnd->activeMdiChild() &&
		m_mainWnd->activeMdiChild()->modalities()->size() > 0 &&
		m_mainWnd->activeMdiChild()->isVolumeDataLoaded();
	dlg_CSVInput dlg(volumeDataAvailable);
	if (m_mainWnd->activeMdiChild())
	{
		auto mdi = m_mainWnd->activeMdiChild();
		QString testCSVFileName = mdi->fileInfo().canonicalPath() + "/" +
				mdi->fileInfo().completeBaseName() + ".csv";
		if (QFile(testCSVFileName).exists())
		{
			dlg.setFileName(testCSVFileName);
			auto type = guessFeatureType(testCSVFileName);
			if (type != InvalidObjectType)
				dlg.setFormat(type == Voids ? iACsvConfig::LegacyVoidFormat : iACsvConfig::LegacyFiberFormat);
		}
		else
			dlg.setPath(mdi->filePath());
	}
	if (dlg.exec() != QDialog::Accepted)
		return;
	iACsvConfig csvConfig = dlg.getConfig();
	if (csvConfig.visType != iACsvConfig::UseVolume)
	{
		if (m_mainWnd->activeMdiChild() && QMessageBox::question(m_mainWnd, "FeatureScout",
			"Load FeatureScout in currently active window (If you choose No, FeatureScout will be opened in a new window)?",
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			m_mdiChild = m_mainWnd->activeMdiChild();
		}
		else
		{
			m_mdiChild = m_mainWnd->createMdiChild(false);
			m_mdiChild->show();
		}
	}
	else
		m_mdiChild = m_mainWnd->activeMdiChild();
	if (!startFeatureScout(csvConfig))
	{
		if (csvConfig.visType != iACsvConfig::UseVolume)
		{
			m_mainWnd->closeMdiChild(m_mdiChild);
			m_mdiChild = nullptr;
			QMessageBox::warning(m_mainWnd, "FeatureScout", "Starting FeatureScout failed! Please check console for detailed error messages!");
		}
	}

}

iAFeatureScoutObjectType iAFeatureScoutModuleInterface::guessFeatureType(QString const & csvFileName)
{
	QFile file( csvFileName );
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		return InvalidObjectType;
	}
	// TODO: create convention, 2nd line of csv file for fibers (pore csv file have this line)
	// Automatic csv file detection
	QTextStream in( &file );
	in.readLine();
	QString item = in.readLine();
	auto returnType = (item == "Voids") ? Voids : Fibers;
	file.close();
	return returnType;
}

void iAFeatureScoutModuleInterface::LoadFeatureScoutWithParams(QString const & csvFileName, MdiChild* mdiChild)
{
	if ( csvFileName.isEmpty() )
		return;
	m_mdiChild = mdiChild;
	auto type = guessFeatureType(csvFileName);
	if (type == InvalidObjectType)
	{
		m_mdiChild->addMsg("CSV-file could not be opened or not a valid FeatureScout file!");
		return;
	}
	iACsvConfig csvConfig = (type != Voids) ?
		iACsvConfig::getLegacyFiberFormat( csvFileName ):
		iACsvConfig::getLegacyPoreFormat( csvFileName );
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
	iARenderSettings FS_RenderSettings = m_mdiChild->renderSettings();
	iAVolumeSettings FS_VolumeSettings = m_mdiChild->volumeSettings();
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

void iAFeatureScoutModuleInterface::LoadFeatureScout(iACsvConfig const & csvConfig, MdiChild * mdiChild)
{
	m_mdiChild = mdiChild;
	startFeatureScout(csvConfig);
}

bool iAFeatureScoutModuleInterface::startFeatureScout(iACsvConfig const & csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
		return false;
	AttachToMdiChild( m_mdiChild );
	connect( m_mdiChild, SIGNAL( closed() ), this, SLOT( onChildClose() ) );
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if ( !attach )
	{
		m_mdiChild->addMsg( "Error while attaching FeatureScout to mdi child window!" );
		return false;
	}
	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;
	if (!csvConfig.curvedFiberFileName.isEmpty())
	{
		readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo);
	}
	attach->init(csvConfig.objectType, csvConfig.fileName, creator.table(), csvConfig.visType, io.getOutputMapping(),
		curvedFiberInfo, csvConfig.cylinderQuality, csvConfig.segmentSkip);
	SetupToolbar();
	m_mdiChild->addStatusMsg(QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	m_mdiChild->addMsg(QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	if (csvConfig.visType == iACsvConfig::UseVolume)
	{
		setFeatureScoutRenderSettings();
		m_mdiChild->addMsg("The render settings of the current child window have been adapted for the volume visualization of FeatureScout!");
	}
	return true;
}

void iAFeatureScoutModuleInterface::FeatureScout_Options()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if ( !attach )
	{
		DEBUG_LOG( "No FeatureScout attachment in current MdiChild!" );
		return;
	}
	QString actionText = qobject_cast<QAction *>(sender())->text();
	int idx = 0;
	if ( actionText.toStdString() == "Length Distribution" ) idx = 7;
	if ( actionText.toStdString() == "Mean Object" ) idx = 4;
	if ( actionText.toStdString() == "Multi Rendering" ) idx = 3;
	if ( actionText.toStdString() == "Orientation Rendering" ) idx = 5;
	if ( actionText.toStdString() == "Activate SPM" ) idx = 6;

	attach->FeatureScout_Options( idx );
	m_mainWnd->statusBar()->showMessage( tr( "FeatureScout options changed to: " ).append( actionText ), 5000 );
}

void iAFeatureScoutModuleInterface::onChildClose()
{
	if (!tlbFeatureScout)
		return;
	auto mdis = m_mainWnd->mdiChildList();
	for (auto mdi : mdis)
	{
		m_mdiChild = mdi;
		iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
		if (attach)
			return;
	}
	m_mainWnd->removeToolBar( tlbFeatureScout );
	delete tlbFeatureScout;
	tlbFeatureScout = nullptr;
}

iAModuleAttachmentToChild * iAFeatureScoutModuleInterface::CreateAttachment( MainWindow* mainWnd, MdiChild * child )
{
	return new iAFeatureScoutAttachment( mainWnd, child );
}

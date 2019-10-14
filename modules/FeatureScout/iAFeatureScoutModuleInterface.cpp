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
#include "ui_CsvInput.h"

#include <iAConsole.h>
#include <iAModalityList.h>
#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module
#include <iAProjectBase.h>
#include <iAProjectRegistry.h>
#include <io/iAFileUtils.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

class iAFeatureScoutProject: public iAProjectBase
{
public:
	static const QString ID;
	iAFeatureScoutProject()
	{}
	virtual ~iAFeatureScoutProject() override
	{}
	void loadProject(QSettings & projectFile, QString const & fileName) override;
	void saveProject(QSettings & projectFile, QString const & fileName) override;
	static QSharedPointer<iAProjectBase> create()
	{
		return QSharedPointer<iAFeatureScoutProject>::create();
	}
	void setOptions(iACsvConfig config)
	{
		m_config = config;
	}
private:
	iACsvConfig m_config;
};

const QString iAFeatureScoutProject::ID("FeatureScout");


void iAFeatureScoutProject::loadProject(QSettings & projectFile, QString const & fileName)
{
	if (!m_mdiChild)
	{
		DEBUG_LOG(QString("Invalid FeatureScout project file '%1': FeatureScout requires an MdiChild, "
			"but UseMdiChild was apparently not specified in this project, as no MdiChild available! "
			"Please report this error, along with the project file, to the open_iA developers!").arg(fileName));
		return;
	}
	m_config.load(projectFile, "CSVFormat");

	QString path(QFileInfo(fileName).absolutePath());
	QString csvFileName = projectFile.value("CSVFileName").toString();
	if (csvFileName.isEmpty())
	{
		DEBUG_LOG(QString("Invalid FeatureScout project file '%1': Empty or missing 'CSVFileName'!").arg(fileName));
		return;
	}
	m_config.fileName = MakeAbsolute(path, csvFileName);
	m_config.curvedFiberFileName = MakeAbsolute(path, projectFile.value("CurvedFileName").toString());
	iAFeatureScoutModuleInterface * featureScout = m_mainWindow->getModuleDispatcher().GetModule<iAFeatureScoutModuleInterface>();
	featureScout->LoadFeatureScout(m_config, m_mdiChild);
	QString layoutName = projectFile.value("Layout").toString();
	if (!layoutName.isEmpty())
		m_mdiChild->loadLayout(layoutName);
}

void iAFeatureScoutProject::saveProject(QSettings & projectFile, QString const & fileName)
{
	m_config.save(projectFile, "CSVFormat");
	QString path(QFileInfo(fileName).absolutePath());
	projectFile.setValue("CSVFileName", MakeRelative(path, m_config.fileName));
	projectFile.setValue("CurvedFileName", MakeRelative(path, m_config.curvedFiberFileName));
	if (m_mdiChild)
		projectFile.setValue("Layout", m_mdiChild->layoutName());
}

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	iAProjectRegistry::addProject<iAFeatureScoutProject>(iAFeatureScoutProject::ID);
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QAction * actionFibreScout = new QAction( QObject::tr("FeatureScout"), nullptr );
	AddActionToMenuAlphabeticallySorted( toolsMenu, actionFibreScout, false );
	connect(actionFibreScout, SIGNAL(triggered()), this, SLOT(FeatureScout()));
	tlbFeatureScout = nullptr;
}

void iAFeatureScoutModuleInterface::FeatureScout()
{
	//auto project = QSharedPointer<iAFeatureScoutProject>::create(m_mainWnd);
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
	AttachToMdiChild(m_mdiChild);
	connect(m_mdiChild, SIGNAL(closed()), this, SLOT(onChildClose()));
	iAFeatureScoutAttachment* attach = GetAttachment<iAFeatureScoutAttachment>();
	if (!attach)
	{
		m_mdiChild->addMsg("Error while attaching FeatureScout to mdi child window!");
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
	auto project = QSharedPointer<iAFeatureScoutProject>::create();
	project->setChild(m_mdiChild);
	project->setOptions(csvConfig);
	m_mdiChild->addProject(iAFeatureScoutProject::ID, project);
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

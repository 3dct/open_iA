/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAFeatureScoutAttachment.h"
#include "iAFeatureScoutToolbar.h"

#include <dlg_CSVInput.h>
#include <iACsvConfig.h>
#include <iACsvIO.h>
#include <iACsvVtkTableCreator.h>

#include <iALog.h>
#include <iAModalityList.h>
#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module
#include <iAProjectBase.h>
#include <iAProjectRegistry.h>
#include <iAFileUtils.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iARenderSettings.h>
#include <iAVolumeSettings.h>

#include <vtkTable.h>
#include <vtkSmartVolumeMapper.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
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
		LOG(lvlError, QString("Invalid FeatureScout project file '%1': FeatureScout requires a child window, "
			"but UseMdiChild was apparently not specified in this project, as no child window available! "
			"Please report this error, along with the project file, to the open_iA developers!").arg(fileName));
		return;
	}
	m_config.load(projectFile, "CSVFormat");

	QString path(QFileInfo(fileName).absolutePath());
	QString csvFileName = projectFile.value("CSVFileName").toString();
	if (csvFileName.isEmpty())
	{
		LOG(lvlError, QString("Invalid FeatureScout project file '%1': Empty or missing 'CSVFileName'!").arg(fileName));
		return;
	}
	m_config.fileName = MakeAbsolute(path, csvFileName);
	if (projectFile.contains("CurvedFileName") && !projectFile.value("CurvedFileName").toString().isEmpty())
	{
		m_config.curvedFiberFileName = MakeAbsolute(path, projectFile.value("CurvedFileName").toString());
	}
	iAFeatureScoutModuleInterface * featureScout = m_mainWindow->moduleDispatcher().module<iAFeatureScoutModuleInterface>();
	featureScout->LoadFeatureScout(m_config, m_mdiChild);
	QString layoutName = projectFile.value("Layout").toString();
	if (!layoutName.isEmpty())
	{
		m_mdiChild->loadLayout(layoutName);
	}
	iAFeatureScoutAttachment* attach = featureScout->attachment<iAFeatureScoutAttachment>(m_mdiChild);
	if (!attach)
	{
		LOG(lvlError, "Error while attaching FeatureScout to mdi child window!");
		return;
	}
	attach->loadProject(projectFile);
}

void iAFeatureScoutProject::saveProject(QSettings & projectFile, QString const & fileName)
{
	m_config.save(projectFile, "CSVFormat");
	QString path(QFileInfo(fileName).absolutePath());
	projectFile.setValue("CSVFileName", MakeRelative(path, m_config.fileName));
	if (!m_config.curvedFiberFileName.isEmpty())
	{
		projectFile.setValue("CurvedFileName", MakeRelative(path, m_config.curvedFiberFileName));
	}
	if (m_mdiChild)
	{
		projectFile.setValue("Layout", m_mdiChild->layoutName());
	}

	iAFeatureScoutModuleInterface* featureScout = m_mainWindow->moduleDispatcher().module<iAFeatureScoutModuleInterface>();
	iAFeatureScoutAttachment* attach = featureScout->attachment<iAFeatureScoutAttachment>(m_mdiChild);
	if (attach)
	{
		attach->saveProject(projectFile);
	}
	else
	{
		LOG(lvlError, "Error: FeatureScoutProject:saveProject called, but no FeatureScout attachment exists!");
	}
}

void iAFeatureScoutModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(FeatureScout);

	iAProjectRegistry::addProject<iAFeatureScoutProject>(iAFeatureScoutProject::ID);
	QAction * actionFibreScout = new QAction(tr("FeatureScout"), m_mainWnd);
	connect(actionFibreScout, &QAction::triggered, this, &iAFeatureScoutModuleInterface::FeatureScout);
	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	submenu->addAction(actionFibreScout);
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
		QString testCSVFileName = pathFileBaseName(mdi->fileInfo()) + ".csv";
		if (QFile(testCSVFileName).exists())
		{
			dlg.setFileName(testCSVFileName);
			auto type = guessFeatureType(testCSVFileName);
			if (type != InvalidObjectType)
			{
				dlg.setFormat(type == Voids ? iACsvConfig::FCVoidFormat : iACsvConfig::FCPFiberFormat);
			}
		}
		else
		{
			dlg.setPath(mdi->filePath());
		}
	}
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();
	bool createdMdi = false;
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
			createdMdi = true;
			m_mdiChild = m_mainWnd->createMdiChild(false);
			m_mdiChild->show();
		}
	}
	else
	{
		m_mdiChild = m_mainWnd->activeMdiChild();
	}
	if (!startFeatureScout(csvConfig) && createdMdi)
	{
		m_mainWnd->closeMdiChild(m_mdiChild);
		m_mdiChild = nullptr;
		QMessageBox::warning(m_mainWnd, "FeatureScout", "Starting FeatureScout failed! Please check console for detailed error messages!");
	}
}

iAObjectType iAFeatureScoutModuleInterface::guessFeatureType(QString const & csvFileName)
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

void iAFeatureScoutModuleInterface::LoadFeatureScoutWithParams(QString const & csvFileName, iAMdiChild* mdiChild)
{
	if (csvFileName.isEmpty())
	{
		return;
	}
	m_mdiChild = mdiChild;
	auto type = guessFeatureType(csvFileName);
	if (type == InvalidObjectType)
	{
		LOG(lvlError, "CSV-file could not be opened or not a valid FeatureScout file!");
		return;
	}
	iACsvConfig csvConfig = (type != Voids) ?
		iACsvConfig::getFCPFiberFormat( csvFileName ):
		iACsvConfig::getFCVoidFormat( csvFileName );
	startFeatureScout(csvConfig);
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

void iAFeatureScoutModuleInterface::LoadFeatureScout(iACsvConfig const & csvConfig, iAMdiChild * mdiChild)
{
	m_mdiChild = mdiChild;
	startFeatureScout(csvConfig);
}

bool iAFeatureScoutModuleInterface::startFeatureScout(iACsvConfig const & csvConfig)
{
	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return false;
	}
	AttachToMdiChild(m_mdiChild);
	iAFeatureScoutAttachment* attach = attachment<iAFeatureScoutAttachment>(m_mdiChild);
	if (!attach)
	{
		LOG(lvlError, "Error while attaching FeatureScout to mdi child window!");
		return false;
	}
	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;
	if (!csvConfig.curvedFiberFileName.isEmpty())
	{
		readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo);
	}
	attach->init(csvConfig.objectType, csvConfig.fileName, creator.table(), csvConfig.visType, io.getOutputMapping(),
		curvedFiberInfo, csvConfig.cylinderQuality, csvConfig.segmentSkip);

	iAFeatureScoutToolbar::addForChild(m_mainWnd, m_mdiChild);
	m_mdiChild->addStatusMsg(QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	LOG(lvlInfo, QString("FeatureScout started (csv: %1)").arg(csvConfig.fileName));
	if (csvConfig.visType == iACsvConfig::UseVolume)
	{
		setFeatureScoutRenderSettings();
		LOG(lvlInfo, "The render settings of the current child window have been adapted for the volume visualization of FeatureScout!");
	}
	auto project = QSharedPointer<iAFeatureScoutProject>::create();
	project->setOptions(csvConfig);
	m_mdiChild->addProject(iAFeatureScoutProject::ID, project);
	return true;
}

iAModuleAttachmentToChild * iAFeatureScoutModuleInterface::CreateAttachment( iAMainWindow* mainWnd, iAMdiChild * child )
{
	return new iAFeatureScoutAttachment( mainWnd, child );
}

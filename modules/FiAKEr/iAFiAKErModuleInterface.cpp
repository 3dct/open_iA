/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFiAKErModuleInterface.h"

#include "iACsvConfig.h"
#include "iAFiberCharData.h"
#include "iAFiAKErController.h"
#include "iAFiAKErAttachment.h"

#include <dlg_commoninput.h>
#include <iAModalityList.h> // only required for initializing mdichild if no volume dataset loaded; should not be needed
#include <iAModuleDispatcher.h>
#include <iAProjectBase.h>
#include <iAProjectRegistry.h>
#include <iAFileUtils.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSettings>

class iAFIAKERProject : public iAProjectBase
{
public:
	iAFIAKERProject():
		m_controller(nullptr)
	{}
	virtual ~iAFIAKERProject() override
	{}
	void loadProject(QSettings & projectFile, QString const & fileName) override
	{
		/*
		// Remove UseiAMdiChild setting altogether, always open iAMdiChild?
		if (projectFile.contains("UseiAMdiChild") && projectFile.value("UseiAMdiChild").toBool() == false)
		{

			QMessageBox::warning(nullptr, "FiAKEr", "Old project file detected (%1). "
				"Due to an implementation change, this file cannot be loaded directly; "
				"please open it in a text editor and remove the ")
			return;
		}
		*/
		if (!m_mdiChild)
		{
			LOG(lvlError, QString("Invalid FIAKER project file '%1': FIAKER requires an iAMdiChild, "
				"but UseiAMdiChild was apparently not specified in this project, as no iAMdiChild available! "
				"Please report this error, along with the project file, to the open_iA developers!").arg(fileName));
			return;
		}
		iAFiAKErModuleInterface * fiaker = m_mainWindow->getModuleDispatcher().GetModule<iAFiAKErModuleInterface>();
		fiaker->setupToolBar();
		fiaker->loadProject(m_mdiChild, projectFile, fileName, this);
	}
	void saveProject(QSettings & projectFile, QString const & fileName) override
	{
		m_controller->saveProject(projectFile, fileName);
	}
	static QSharedPointer<iAProjectBase> create()
	{
		return QSharedPointer<iAFIAKERProject>::create();
	}
	void setController(iAFiAKErController* controller)
	{
		m_controller = controller;
	}
private:
	iAFiAKErController* m_controller;
};

namespace
{
	const QString LastFormatKey("FIAKER/LastFormat");
	const QString LastTimeStepOffsetKey("FIAKER/LastTimeStepOffsetKey");
	const QString LastPathKey("FIAKER/LastPath");
	const QString LastUseStepData("FIAKER/LastUseStepData");
	const QString LastShowPreviews("FIAKER/LastShowPreviews");
}

void iAFiAKErModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAProjectRegistry::addProject<iAFIAKERProject>(iAFiAKErController::FIAKERProjectID);

	QAction * actionFiAKEr = new QAction(tr("Open Results Folder"), m_mainWnd);
	actionFiAKEr->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R, Qt::Key_O));
	connect(actionFiAKEr, &QAction::triggered, this, &iAFiAKErModuleInterface::startFiAKEr );

	QAction * actionFiAKErProject = new QAction(tr("Load Project (for .fpf; for .iaproj use File->Open)"), m_mainWnd);
	actionFiAKErProject->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R, Qt::Key_P));
	connect(actionFiAKErProject, &QAction::triggered, this, &iAFiAKErModuleInterface::loadFiAKErProject);

	QMenu* fiakerMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("FiAKEr"), false);
	fiakerMenu->addAction(actionFiAKEr);
	fiakerMenu->addAction(actionFiAKErProject);

	QSettings s;
	m_lastFormat = s.value(LastFormatKey, "").toString();
	m_lastPath = s.value(LastPathKey, m_mainWnd->path()).toString();
	m_lastUseStepData = s.value(LastUseStepData, true).toBool();
	m_lastShowPreviews = s.value(LastShowPreviews, true).toBool();
	bool ok;
	m_lastTimeStepOffset = s.value(LastTimeStepOffsetKey, 0).toDouble(&ok);
	if (!ok)
	{
		LOG(lvlError, "FIAKER start: Invalid m_lastTimeStepOffset stored in settings!");
	}
}

void iAFiAKErModuleInterface::SaveSettings() const
{
	QSettings s;
	s.setValue(LastFormatKey, m_lastFormat);
	s.setValue(LastPathKey, m_lastPath);
	s.setValue(LastTimeStepOffsetKey, m_lastTimeStepOffset);
	s.setValue(LastUseStepData, m_lastUseStepData);
	s.setValue(LastShowPreviews, m_lastShowPreviews);
}

void iAFiAKErModuleInterface::startFiAKEr()
{
	setupToolBar();
	iAMdiChild* mdiChild(nullptr);
	bool createdMdi = false;
	if (m_mainWnd->activeMdiChild() && QMessageBox::question(m_mainWnd, "FIAKER",
		"Load FIAKER in currently active window (If you choose No, FIAKER will be opened in a new window)?",
		QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
	{
		mdiChild = m_mainWnd->activeMdiChild();
	}
	else
	{
		createdMdi = true;
		mdiChild = m_mainWnd->createMdiChild(false);
		mdiChild->show();
	}
	QStringList parameterNames = QStringList()
		<< ";Result folder"
		<< "+CSV cormat"
		<< "#Step coordinate shift"
		<< "$Use step data"
		<< "$Show result previews in list";
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iAFiberResultsCollection::SimpleFormat))
	{
		formatEntries.append(iAFiberResultsCollection::SimpleFormat);
	}
	if (!formatEntries.contains(iAFiberResultsCollection::LegacyFormat))
	{
		formatEntries.append(iAFiberResultsCollection::LegacyFormat);
	}
	if (!formatEntries.contains(iACsvConfig::LegacyFiberFormat))
	{
		formatEntries.append(iACsvConfig::LegacyFiberFormat);
	}
	if (!formatEntries.contains(iACsvConfig::LegacyVoidFormat))
	{
		formatEntries.append(iACsvConfig::LegacyVoidFormat);
	}
	for (int i = 0; i < formatEntries.size(); ++i)
	{
		if (formatEntries[i] == m_lastFormat)
		{
			formatEntries[i] = "!" + formatEntries[i];
		}
	}

	QList<QVariant> values;
	values << m_lastPath << formatEntries << m_lastTimeStepOffset << m_lastUseStepData << m_lastShowPreviews;

	QString descr("Starts FIAKER, a comparison tool for results from fiber reconstruction algorithms.<br/>"
		"Choose a <em>Result folder</em> containing two or more fiber reconstruction results in .csv format. "
		"Under <em>CSV format</em>, select the format in which data is stored in your .csv files. "
		"You can test / modify the format via Tools->FeatureScout (just select a single one of your .csv's there, "
		"and refine the format until it is shown properly, then store the format, then use it here. "
		"<em>Step coordinate shift</em> defines a shift that is applied to all coordinates, in each dimension (x, y and z) "
		"for the optimization step files. For the final results, "
		"there is a similar setting available via the .csv format specification, see above. "
		"<em>Use step data</em> determines whether FIAKER immediately uses information on curved files from the last step; "
		"if unchecked, FIAKER will initially show the separate curved representation for the final result.<br/>"
		"For more information on FIAKER, see the corresponding publication: "
		"Bernhard Fröhler, Tim Elberfeld, Torsten Möller, Hans-Christian Hege, Johannes Weissenböck, "
		"Jan De Beenhouwer, Jan Sijbers, Johann Kastner and Christoph Heinzl, "
		"A Visual Tool for the Analysis of Algorithms for Tomographic Fiber Reconstruction in Materials Science, "
		"2019, Computer Graphics Forum 38 (3), <a href=\"https://doi.org/10.1111/cgf.13688\">doi: 10.1111/cgf.13688</a>.");
	dlg_commoninput dlg(m_mainWnd, "Start FIAKER", parameterNames, values, descr);
	if (dlg.exec() != QDialog::Accepted || dlg.getText(0).isEmpty())
	{
		if (createdMdi)
		{
			m_mainWnd->closeMdiChild(mdiChild);
		}
		return;
	}
	m_lastPath = dlg.getText(0);
	m_lastFormat = dlg.getComboBoxValue(1);
	m_lastTimeStepOffset = dlg.getDblValue(2);
	m_lastUseStepData = dlg.getCheckValue(3);
	m_lastShowPreviews = dlg.getCheckValue(4);
	//cmbbox_Format->addItems(formatEntries);

	AttachToMdiChild(mdiChild);
	iAFiAKErAttachment* attach = GetAttachment<iAFiAKErAttachment>();
	m_mainWnd->setPath(m_lastPath);
	if (createdMdi)
	{
		mdiChild->setWindowTitle(QString("FIAKER (%1)").arg(m_lastPath));
	}
	auto project = QSharedPointer<iAFIAKERProject>::create();
	project->setController(attach->controller());
	mdiChild->addProject(iAFiAKErController::FIAKERProjectID, project);
	attach->controller()->start(m_lastPath, getCsvConfig(m_lastFormat), m_lastTimeStepOffset, m_lastUseStepData, m_lastShowPreviews);
}

void iAFiAKErModuleInterface::loadFiAKErProject()
{
	setupToolBar();
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		iAFiAKErController::FIAKERProjectID, m_mainWnd->path(), "FIAKER Project file (*.fpf);;");
	if (fileName.isEmpty())
	{
		return;
	}
	iAMdiChild* newChild = m_mainWnd->createMdiChild(false);
	newChild->show();
	QSettings projectFile(fileName, QSettings::IniFormat);
	projectFile.setIniCodec("UTF-8");
	auto project = QSharedPointer<iAFIAKERProject>::create();
	project->setMainWindow(m_mainWnd);
	project->setChild(newChild);
	loadProject(newChild, projectFile, fileName, project.data());
	newChild->addProject(iAFiAKErController::FIAKERProjectID, project);
}

void iAFiAKErModuleInterface::loadProject(iAMdiChild* mdiChild, QSettings const& projectFile, QString const& fileName, iAFIAKERProject* project)
{
	if (mdiChild->modalities()->size() == 0)
	{ // if no other data loaded yet, we need to make suare mdichild is initialized:
		mdiChild->displayResult(mdiChild->windowTitle(), nullptr, nullptr);
	}
	AttachToMdiChild(mdiChild);
	iAFiAKErAttachment* attach = GetAttachment<iAFiAKErAttachment>();
	auto controller = attach->controller();
	project->setController(controller);
	m_mainWnd->setPath(m_lastPath);
	iASettings projectSettings = mapFromQSettings(projectFile);
	connect(controller, &iAFiAKErController::setupFinished, [controller, projectSettings]
		{
			controller->loadReference(projectSettings);
		});
	controller->loadProject(projectFile, fileName);
}

iAModuleAttachmentToChild* iAFiAKErModuleInterface::CreateAttachment(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return new iAFiAKErAttachment(mainWnd, child);
}

void iAFiAKErModuleInterface::setupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAFiAKErToolBar("FiAKEr Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, &iAFiAKErModuleInterface::toggleDockWidgetTitleBars);
	connect(m_toolbar->action_ToggleSettings, &QAction::triggered, this, &iAFiAKErModuleInterface::toggleSettings);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

void iAFiAKErModuleInterface::toggleDockWidgetTitleBars()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAFiAKErAttachment* attach = GetAttachment<iAFiAKErAttachment>();
	if (!attach)
	{
		return;
	}
	attach->controller()->toggleDockWidgetTitleBars();
}

void iAFiAKErModuleInterface::toggleSettings()
{
	m_mdiChild = m_mainWnd->activeMdiChild();
	iAFiAKErAttachment* attach = GetAttachment<iAFiAKErAttachment>();
	if (!attach)
	{
		return;
	}
	attach->controller()->toggleSettings();
}

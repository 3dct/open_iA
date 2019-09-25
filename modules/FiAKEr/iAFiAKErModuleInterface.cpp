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
#include "iAFiAKErModuleInterface.h"

#include "iACsvConfig.h"
#include "iAFiberCharData.h"
#include "iAFiAKErController.h"

#include <dlg_commoninput.h>
#include <iAModuleDispatcher.h>
#include <iAProjectBase.h>
#include <iAProjectRegistry.h>
#include <mainwindow.h>

#include <QAction>
#include <QFileDialog>
#include <QMdiSubWindow>


class iAFIAKERProject : public iAProjectBase
{
public:
	iAFIAKERProject()
	{}
	virtual ~iAFIAKERProject() override
	{}
	void loadProject(QSettings & projectFile, QString const & fileName) override
	{
		iAFiAKErModuleInterface * fiaker = m_mainWindow->getModuleDispatcher().GetModule<iAFiAKErModuleInterface>();
		fiaker->setupToolBar();
		iAFiAKErController::loadProject(m_mainWindow, projectFile, fileName);
	}
	//! not required at the moment, since this is currently done by
	//! iAFiAKErController::doSaveProject overwriting iASavableProject::doSaveProject
	void saveProject(QSettings & projectFile) override
	{}
	static QSharedPointer<iAProjectBase> create()
	{
		return QSharedPointer<iAFIAKERProject>::create();
	}
	void setOptions(iACsvConfig config)
	{
		m_config = config;
	}
private:
	iACsvConfig m_config;
};

void iAFiAKErModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	iAProjectRegistry::addProject<iAFIAKERProject>(iAFiAKErController::FIAKERProjectID);
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QMenu * fiakerMenu = getMenuWithTitle(toolsMenu, tr("FiAKEr"), false);
	QAction * actionFiAKEr = new QAction( "Open Results Folder", nullptr );
	actionFiAKEr->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R, Qt::Key_O));
	AddActionToMenuAlphabeticallySorted(fiakerMenu, actionFiAKEr, false );
	connect(actionFiAKEr, &QAction::triggered, this, &iAFiAKErModuleInterface::startFiAKEr );
	QAction * actionFiAKErProject = new QAction("Load Project", nullptr);
	actionFiAKErProject->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R, Qt::Key_P));
	AddActionToMenuAlphabeticallySorted(fiakerMenu, actionFiAKErProject, false);
	connect(actionFiAKErProject, &QAction::triggered, this, &iAFiAKErModuleInterface::loadFiAKErProject);
}

void iAFiAKErModuleInterface::startFiAKEr()
{
	setupToolBar();
	QString path = QFileDialog::getExistingDirectory(m_mainWnd, "Choose Folder containing Result csv", m_mainWnd->path());
	if (path.isEmpty())
		return;
	
	auto explorer = new iAFiAKErController(m_mainWnd);
	QStringList parameterNames = QStringList() << "+CSV Format" << "#Step Coordinate Shift";
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iAFiberResultsCollection::SimpleFormat))
		formatEntries.append(iAFiberResultsCollection::SimpleFormat);
	if (!formatEntries.contains(iAFiberResultsCollection::LegacyFormat))
		formatEntries.append(iAFiberResultsCollection::LegacyFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyFiberFormat))
		formatEntries.append(iACsvConfig::LegacyFiberFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyVoidFormat))
		formatEntries.append(iACsvConfig::LegacyVoidFormat);
	QList<QVariant> values;
	values << formatEntries << 0;
	dlg_commoninput dlg(m_mainWnd, "Choose CSV Format", parameterNames, values);
	if (dlg.exec() != QDialog::Accepted)
		return;
	QString configName = dlg.getComboBoxValue(0);
	double stepShift = dlg.getDblValue(1);
	//cmbbox_Format->addItems(formatEntries);
	m_mainWnd->addSubWindow(explorer);
	auto project = QSharedPointer<iAFIAKERProject>::create();
	explorer->start(path, configName, stepShift);
}

void iAFiAKErModuleInterface::loadFiAKErProject()
{
	setupToolBar();
	iAFiAKErController::loadAnalysis(m_mainWnd, m_mainWnd->path());
}

void iAFiAKErModuleInterface::setupToolBar()
{
	if (m_toolbar)
		return;
	m_toolbar = new iAFiAKErToolBar("FiAKEr Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, SIGNAL(triggered()), this, SLOT(toggleDockWidgetTitleBars()));
	connect(m_toolbar->action_ToggleSettings, SIGNAL(triggered()), this, SLOT(toggleSettings()));
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

namespace
{
	iAFiAKErController* getActiveFiAKEr(MainWindow* mainWnd)
	{
		auto curWnd = mainWnd->mdiArea->currentSubWindow();
		return (curWnd) ? qobject_cast<iAFiAKErController*>(curWnd->widget()) : nullptr;
	}
}

void iAFiAKErModuleInterface::toggleDockWidgetTitleBars()
{
	auto fiakerController = getActiveFiAKEr(m_mainWnd);
	if (!fiakerController)
		return;
	fiakerController->toggleDockWidgetTitleBars();
}

void iAFiAKErModuleInterface::toggleSettings()
{
	auto fiakerController = getActiveFiAKEr(m_mainWnd);
	if (!fiakerController)
		return;
	fiakerController->toggleSettings();
}

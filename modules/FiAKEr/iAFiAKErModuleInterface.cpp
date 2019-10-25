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
#include <QSettings>

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
	void saveProject(QSettings & projectFile, QString const & fileName) override
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

namespace
{
	const QString LastFormatKey("FIAKER/LastFormat");
	const QString LastTimeStepOffsetKey("FIAKER/LastTimeStepOffsetKey");
	const QString LastPathKey("FIAKER/LastPath");
	const QString LastUseStepData("FIAKER/LastUseStepData");
}

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
	QAction * actionFiAKErProject = new QAction("Load Project (for .fpf; for .opf use File->Open)", nullptr);
	actionFiAKErProject->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R, Qt::Key_P));
	AddActionToMenuAlphabeticallySorted(fiakerMenu, actionFiAKErProject, false);
	connect(actionFiAKErProject, &QAction::triggered, this, &iAFiAKErModuleInterface::loadFiAKErProject);

	QSettings s;
	m_lastFormat = s.value(LastFormatKey, "").toString();
	m_lastPath = s.value(LastPathKey, m_mainWnd->path()).toString();
	m_lastUseStepData = s.value(LastUseStepData, true).toBool();
	bool ok;
	m_lastTimeStepOffset = s.value(LastTimeStepOffsetKey, 0).toDouble(&ok);
	if (!ok)
		DEBUG_LOG("FIAKER start: Invalid m_lastTimeStepOffset stored in settings!");
}

void iAFiAKErModuleInterface::SaveSettings() const
{
	QSettings s;
	s.setValue(LastFormatKey, m_lastFormat);
	s.setValue(LastPathKey, m_lastPath);
	s.setValue(LastTimeStepOffsetKey, m_lastTimeStepOffset);
	s.setValue(LastUseStepData, m_lastUseStepData);
}

void iAFiAKErModuleInterface::startFiAKEr()
{
	setupToolBar();
	auto explorer = new iAFiAKErController(m_mainWnd);
	QStringList parameterNames = QStringList()
		<< ";Result folder"
		<< "+CSV cormat"
		<< "#Step coordinate shift"
		<< "$Use step data";
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iAFiberResultsCollection::SimpleFormat))
		formatEntries.append(iAFiberResultsCollection::SimpleFormat);
	if (!formatEntries.contains(iAFiberResultsCollection::LegacyFormat))
		formatEntries.append(iAFiberResultsCollection::LegacyFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyFiberFormat))
		formatEntries.append(iACsvConfig::LegacyFiberFormat);
	if (!formatEntries.contains(iACsvConfig::LegacyVoidFormat))
		formatEntries.append(iACsvConfig::LegacyVoidFormat);
	for (int i = 0; i < formatEntries.size(); ++i)
		if (formatEntries[i] == m_lastFormat)
			formatEntries[i] = "!" + formatEntries[i];
		
	QList<QVariant> values;
	values << m_lastPath << formatEntries << m_lastTimeStepOffset << m_lastUseStepData;
	
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
	if (dlg.exec() != QDialog::Accepted)
		return;
	if (dlg.getText(0).isEmpty())
		return;
	m_lastPath = dlg.getText(0);
	m_lastFormat = dlg.getComboBoxValue(1);
	m_lastTimeStepOffset = dlg.getDblValue(2);
	m_lastUseStepData = dlg.getCheckValue(3);
	//cmbbox_Format->addItems(formatEntries);
	m_mainWnd->addSubWindow(explorer);
	m_mainWnd->setPath(m_lastPath);
	auto project = QSharedPointer<iAFIAKERProject>::create();
	explorer->start(m_lastPath, getCsvConfig(m_lastFormat), m_lastTimeStepOffset, m_lastUseStepData);
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

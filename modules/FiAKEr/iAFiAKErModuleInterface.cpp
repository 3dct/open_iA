// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFiAKErModuleInterface.h"

#include "iACsvConfig.h"
#include "iAFiberResult.h"
#include "iAFiAKErController.h"
#include "iAFiAKErTool.h"

#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAParameterDlg.h>
#include <iATool.h>
#include <iAToolRegistry.h>

#include <iAAttributeDescriptor.h>    // for selectOption
#include <iAFileUtils.h>
#include <iALog.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QSettings>

namespace
{
	const QString LastFormatKey("FIAKER/LastFormat");
	const QString LastTimeStepOffsetKey("FIAKER/LastTimeStepOffsetKey");
	const QString LastPathKey("FIAKER/LastPath");
	const QString LastUseStepData("FIAKER/LastUseStepData");
	const QString LastShowPreviews("FIAKER/LastShowPreviews");
	const QString LastShowCharts("FIAKER/LastShowCharts");
}

void iAFiAKErModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	iAToolRegistry::addTool(iAFiAKErController::FIAKERToolID, iAFiAKErTool::create);

	QAction * actionFiAKEr = new QAction(tr("Start FIAKER"), m_mainWnd);
	actionFiAKEr->setShortcut(QKeySequence(QKeyCombination(Qt::ALT, Qt::Key_R), QKeyCombination(Qt::Key_O)));
	connect(actionFiAKEr, &QAction::triggered, this, &iAFiAKErModuleInterface::startFiAKEr );

	QAction* actionFiAKErProject = new QAction(tr("Load FIAKER (old .fpf files)"), m_mainWnd);
	actionFiAKErProject->setShortcut(QKeySequence(QKeyCombination(Qt::ALT, Qt::Key_R), QKeyCombination(Qt::Key_P)));
	connect(actionFiAKErProject, &QAction::triggered, this, &iAFiAKErModuleInterface::loadFiAKErProject);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	submenu->addAction(actionFiAKEr);
	submenu->addAction(actionFiAKErProject);

	QSettings s;
	m_lastFormat = s.value(LastFormatKey, "").toString();
	m_lastPath = s.value(LastPathKey, m_mainWnd->path()).toString();
	m_lastUseStepData = s.value(LastUseStepData, true).toBool();
	m_lastShowPreviews = s.value(LastShowPreviews, true).toBool();
	m_lastShowCharts = s.value(LastShowCharts, true).toBool();
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
	s.setValue(LastShowCharts, m_lastShowCharts);
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
	}
	QStringList formatEntries = iACsvConfig::getListFromRegistry();
	if (!formatEntries.contains(iAFiberResultsCollection::SimpleFormat))
	{
		formatEntries.append(iAFiberResultsCollection::SimpleFormat);
	}
	if (!formatEntries.contains(iAFiberResultsCollection::FiakerFCPFormat))
	{
		formatEntries.append(iAFiberResultsCollection::FiakerFCPFormat);
	}
	if (!formatEntries.contains(iACsvConfig::FCPFiberFormat))
	{
		formatEntries.append(iACsvConfig::FCPFiberFormat);
	}
	if (!formatEntries.contains(iACsvConfig::FCVoidFormat))
	{
		formatEntries.append(iACsvConfig::FCVoidFormat);
	}
	selectOption(formatEntries, m_lastFormat);
	iAAttributes params;
	addAttr(params, "Result folder", iAValueType::Folder, m_lastPath);
	addAttr(params, "CSV cormat", iAValueType::Categorical, formatEntries);
	addAttr(params, "Step coordinate shift", iAValueType::Continuous, m_lastTimeStepOffset);
	addAttr(params, "Use step data", iAValueType::Boolean, m_lastUseStepData);
	addAttr(params, "Show mini previews in result list", iAValueType::Boolean, m_lastShowPreviews);
	addAttr(params, "Show distribution charts in result list", iAValueType::Boolean, m_lastShowCharts);
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
		"For more information on FIAKER, see the corresponding publications:"
		"<ul><li>B. Fröhler, T. Elberfeld, T. Möller, H.-C. Hege, J. Weissenböck, "
		"J. De Beenhouwer, J. Sijbers, J. Kastner, and C. Heinzl, "
		"A Visual Tool for the Analysis of Algorithms for Tomographic Fiber Reconstruction in Materials Science, "
		"Computer Graphics Forum 38 (3), 2019, pp. 273-283, doi: <a href=\"https://doi.org/10.1111/cgf.13688\">10.1111/cgf.13688</a>.</li>"
		"<li>B. Fröhler, T. Elberfeld, T. Möller, H.-C. Hege, J. De Beenhouwer, J. Sijbers, J. Kastner, and Christoph Heinzl, "
		"Analysis and comparison of algorithms for the tomographic reconstruction of curved fibres, "
		"Nondestructive Testing and Evaluation 35 (3), 2020, pp. 328–341, "
		"doi: <a href=\"https://doi.org/10.1080/10589759.2020.1774583\">10.1080/10589759.2020.1774583</a>.</li></ul>");
	iAParameterDlg dlg(m_mainWnd, "Start FIAKER", params, descr);
	if (dlg.exec() != QDialog::Accepted || dlg.parameterValues()["Result folder"].toString().isEmpty())
	{
		if (createdMdi)
		{
			m_mainWnd->closeMdiChild(mdiChild);
		}
		return;
	}
	auto values = dlg.parameterValues();
	m_lastPath = values["Result folder"].toString();
	m_lastFormat = values["CSV cormat"].toString();
	m_lastTimeStepOffset = values["Step coordinate shift"].toDouble();
	m_lastUseStepData = values["Use step data"].toBool();
	m_lastShowPreviews = values["Show mini previews in result list"].toBool();
	m_lastShowCharts = values["Show distribution charts in result list"].toBool();

	m_mainWnd->setPath(m_lastPath);
	if (createdMdi)
	{
		mdiChild->setWindowTitle(QString("FIAKER (%1)").arg(m_lastPath));
	}
	auto tool = iAFiAKErTool::create(m_mainWnd, mdiChild);
	mdiChild->addTool(iAFiAKErController::FIAKERToolID, tool);
	dynamic_cast<iAFiAKErTool*>(tool.get())->controller()->start(m_lastPath, getCsvConfig(m_lastFormat), m_lastTimeStepOffset,
		m_lastUseStepData, m_lastShowPreviews, m_lastShowCharts);
}

void iAFiAKErModuleInterface::loadFiAKErProject()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		iAFiAKErController::FIAKERToolID, m_mainWnd->path(), "FIAKER Project file (*.fpf);;All files (*)");
	if (fileName.isEmpty())
	{
		return;
	}
	iAMdiChild* newChild = m_mainWnd->createMdiChild(false);
	newChild->show();
	QSettings projectFile(fileName, QSettings::IniFormat);
	auto tool = iAFiAKErTool::create(m_mainWnd, newChild);
	newChild->addTool(iAFiAKErController::FIAKERToolID, tool);
	tool->loadState(projectFile, fileName);
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
	iAFiAKErTool* tool = getTool<iAFiAKErTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		return;
	}
	tool->controller()->toggleDockWidgetTitleBars();
}

void iAFiAKErModuleInterface::toggleSettings()
{
	iAFiAKErTool* tool = getTool<iAFiAKErTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		return;
	}
	tool->controller()->toggleSettings();
}

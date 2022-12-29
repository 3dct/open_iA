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
#include "iAUncertaintyModuleInterface.h"

#include "iAUncertaintyTool.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAParameterDlg.h>

#include <QFileDialog>
#include <QMenu>

void iAUncertaintyModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionUncertainty = new QAction(tr("Uncertainty Exploration"), m_mainWnd);
	connect(actionUncertainty, &QAction::triggered, this, &iAUncertaintyModuleInterface::uncertaintyExploration);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionUncertainty);
}

void iAUncertaintyModuleInterface::uncertaintyExploration()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Load Ensemble"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->filePath(): QString(),
		tr("Image Analysis Ensemble (*.iae );;All files (*)") );
	if (!fileName.isEmpty())
	{
		loadEnsemble(fileName);
	}
}

void iAUncertaintyModuleInterface::loadEnsemble(QString const & fileName)
{
	setupToolBar();
	auto child = m_mainWnd->createMdiChild(false);
	auto tool = std::make_shared<iAUncertaintyTool>(m_mainWnd, child);
	child->addTool("Uncertainty", tool);
	child->show();
	if (!tool->loadEnsemble(fileName))
	{
		return;
	}
}

void iAUncertaintyModuleInterface::setupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAUncertaintyToolbar("Uncertainty Exploration Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, &iAUncertaintyModuleInterface::toggleDockWidgetTitleBars);
	m_toolbar->action_ToggleSettings->setCheckable(true);
	m_toolbar->action_ToggleSettings->setChecked(true);
	connect(m_toolbar->action_ToggleSettings, &QAction::triggered, this, &iAUncertaintyModuleInterface::toggleSettings);
	connect(m_toolbar->action_CalculateNewSubEnsemble, &QAction::triggered, this, &iAUncertaintyModuleInterface::calculateNewSubEnsemble);
	connect(m_toolbar->action_WriteFullDataFile, &QAction::triggered, this, &iAUncertaintyModuleInterface::writeFullDataFile);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

void iAUncertaintyModuleInterface::toggleDockWidgetTitleBars()
{
	auto tool = getTool<iAUncertaintyTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	tool->toggleDockWidgetTitleBars();
}

void iAUncertaintyModuleInterface::toggleSettings()
{
	auto tool = getTool<iAUncertaintyTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	tool->toggleSettings();
}

void iAUncertaintyModuleInterface::calculateNewSubEnsemble()
{
	auto tool = getTool<iAUncertaintyTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	tool->calculateNewSubEnsemble();
}

void iAUncertaintyModuleInterface::writeFullDataFile()
{
	auto tool = getTool<iAUncertaintyTool>(m_mainWnd->activeMdiChild());
	if (!tool)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	QString fileName = QFileDialog::getSaveFileName(m_mainWnd,
		tr("Save Full Data file"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->filePath() : QString(),
		tr("SVM file format (*.svm);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, "Write original data values", iAValueType::Boolean, true);
	addAttr(params, "Write Member Labels", iAValueType::Boolean, true);
	addAttr(params, "Write Member Probabilities", iAValueType::Boolean, true);
	addAttr(params, "Write Ensemble Uncertainties", iAValueType::Boolean, true);
	iAParameterDlg dlg(m_mainWnd, "Write parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto val = dlg.parameterValues();
	tool->writeFullDataFile(fileName, val["Write original data values"].toBool(), val["Write Member Labels"].toBool(),
		val["Write Member Probabilities"].toBool(), val["Write Ensemble Uncertainties"].toBool());
}

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

#include "iAEntropy.h"
#include "iACSVtoMHD.h"
#include "iAUncertaintyAttachment.h"

#include <iALog.h>
#include <iAFilterRegistry.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAParameterDlg.h>

#include <QFileDialog>
#include <QMenu>

void iAUncertaintyModuleInterface::Initialize()
{
	REGISTER_FILTER(iAEntropy);
	REGISTER_FILTER(iACSVtoMHD);
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionUncertainty = new QAction(tr("Uncertainty Exploration"), m_mainWnd);
	connect(actionUncertainty, &QAction::triggered, this, &iAUncertaintyModuleInterface::UncertaintyExploration);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionUncertainty);
}


iAModuleAttachmentToChild* iAUncertaintyModuleInterface::CreateAttachment(iAMainWindow* mainWnd, iAMdiChild * child)
{
	iAUncertaintyAttachment* result = iAUncertaintyAttachment::Create( mainWnd, child);
	return result;
}


void iAUncertaintyModuleInterface::UncertaintyExploration()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Load Ensemble"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->filePath(): QString(),
		tr("Image Analysis Ensemble (*.iae );;All files (*)") );
	if (!fileName.isEmpty())
	{
		LoadEnsemble(fileName);
	}
}

void iAUncertaintyModuleInterface::LoadEnsemble(QString const & fileName)
{
	SetupToolBar();
	auto child = m_mainWnd->createMdiChild(false);
	bool result = AttachToMdiChild(child);
	iAUncertaintyAttachment* attach = attachment<iAUncertaintyAttachment>(child);
	if (!result || !attach)
	{
		LOG(lvlError, "Uncertainty exploration could not be initialized!");
		return;
	}
	child->show();
	if (!attach->LoadEnsemble(fileName))
	{
		return;
	}
}

void iAUncertaintyModuleInterface::SetupToolBar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAUncertaintyToolbar("Uncertainty Exploration Toolbar");
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, &iAUncertaintyModuleInterface::ToggleDockWidgetTitleBars);
	m_toolbar->action_ToggleSettings->setCheckable(true);
	m_toolbar->action_ToggleSettings->setChecked(true);
	connect(m_toolbar->action_ToggleSettings, &QAction::triggered, this, &iAUncertaintyModuleInterface::ToggleSettings);
	connect(m_toolbar->action_CalculateNewSubEnsemble, &QAction::triggered, this, &iAUncertaintyModuleInterface::CalculateNewSubEnsemble);
	connect(m_toolbar->action_WriteFullDataFile, &QAction::triggered, this, &iAUncertaintyModuleInterface::WriteFullDataFile);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);
}

void iAUncertaintyModuleInterface::ToggleDockWidgetTitleBars()
{
	iAUncertaintyAttachment* attach = attachment<iAUncertaintyAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleDockWidgetTitleBars();
}

void iAUncertaintyModuleInterface::ToggleSettings()
{
	iAUncertaintyAttachment* attach = attachment<iAUncertaintyAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->ToggleSettings();
}

void iAUncertaintyModuleInterface::CalculateNewSubEnsemble()
{
	iAUncertaintyAttachment* attach = attachment<iAUncertaintyAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
	{
		LOG(lvlError, "Uncertainty exploration was not loaded properly!");
		return;
	}
	attach->CalculateNewSubEnsemble();
}

void iAUncertaintyModuleInterface::WriteFullDataFile()
{
	iAUncertaintyAttachment* attach = attachment<iAUncertaintyAttachment>(m_mainWnd->activeMdiChild());
	if (!attach)
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
	iAParameterDlg::ParamListT params;
	addParameter(params, "Write original data values", iAValueType::Boolean, true);
	addParameter(params, "Write Member Labels", iAValueType::Boolean, true);
	addParameter(params, "Write Member Probabilities", iAValueType::Boolean, true);
	addParameter(params, "Write Ensemble Uncertainties", iAValueType::Boolean, true);
	iAParameterDlg dlg(m_mainWnd, "Write parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto val = dlg.parameterValues();
	attach->WriteFullDataFile(fileName, val["Write original data values"].toBool(), val["Write Member Labels"].toBool(),
		val["Write Member Probabilities"].toBool(), val["Write Ensemble Uncertainties"].toBool());
}

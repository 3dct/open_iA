// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureAnalyzerModuleInterface.h"

#include "iADataFolderDialog.h"
#include "iAFeatureAnalyzerTool.h"
#include "iAFeatureAnalyzer.h"

//#include <defines.h>
#include <iAToolRegistry.h>
#include <iAMainWindow.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QTime>

void iAFeatureAnalyzerModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(FeatureAnalyzer);
	iAToolRegistry::addTool(iAFeatureAnalyzerTool::ID, iAFeatureAnalyzerTool::create);

	QAction * actionRunPA = new QAction(tr("Start FeatureAnalyzer"), m_mainWnd);
	connect(actionRunPA, &QAction::triggered, this, &iAFeatureAnalyzerModuleInterface::launchFeatureAnalyzer);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	submenu->addAction( actionRunPA );
}

void iAFeatureAnalyzerModuleInterface::launchFeatureAnalyzer()
{
	// TODO: make m_featureAnalyzer a list or something?
	if (m_featureAnalyzer)
	{
		QMessageBox::information(m_mainWnd, "FeatureAnalyzer", "Current implementation only permits a single instance of FeatureAnalyzer to be started!");
		return;
	}
	iADataFolderDialog* dlg = new iADataFolderDialog(m_mainWnd);
	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}
	startFeatureAnalyzer(dlg->ResultsFolderName(), dlg->DatasetsFolderName());
}

void iAFeatureAnalyzerModuleInterface::startFeatureAnalyzer(QString const & resultsFolderName, QString const & datasetsFolderName)
{
	m_featureAnalyzer = new iAFeatureAnalyzer(m_mainWnd, resultsFolderName, datasetsFolderName, m_mainWnd );
	m_mainWnd->addSubWindow(m_featureAnalyzer);
	m_featureAnalyzer->LoadStateAndShow(); //show();
}

/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFeatureAnalyzerModuleInterface.h"

#include "iADataFolderDialog.h"
#include "iAFeatureAnalyzerProject.h"
#include "iAFeatureAnalyzer.h"

//#include <defines.h>
#include <iAProjectRegistry.h>
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
	iAProjectRegistry::addProject<iAFeatureAnalyzerProject>(iAFeatureAnalyzerProject::ID);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
	qsrand(QTime::currentTime().msec());
#endif

	QAction * actionRunPA = new QAction(tr("Analyze Segmentations"), m_mainWnd);
	connect(actionRunPA, &QAction::triggered, this, &iAFeatureAnalyzerModuleInterface::launchFeatureAnalyzer);

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("FeatureAnalyzer"));
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

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
#include "iAGEMSeModuleInterface.h"

#include "iAGEMSeAttachment.h"
#include "iAGEMSeProject.h"
#include "iASEAFile.h"

#include <dlg_modalities.h>
#include <iAConsole.h>
#include <iAModality.h>
#include <iAProjectRegistry.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QFileDialog>

#include <cassert>


iAGEMSeModuleInterface::iAGEMSeModuleInterface():
	m_toolbar(0)
{}

void iAGEMSeModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;

	iAProjectRegistry::addProject<iAGEMSeProject>(iAGEMSeProject::ID);
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QMenu * menuEnsembles = getMenuWithTitle( toolsMenu, tr( "Image Ensembles" ), false );
	
	QAction * actionGEMSe = new QAction( tr("GEMSe"), nullptr);
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionGEMSe, true);
	connect(actionGEMSe, &QAction::triggered, this, &iAGEMSeModuleInterface::startGEMSe);

	QAction * actionPreCalculated = new QAction( tr("GEMSe - Load Ensemble (old)"), nullptr );
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionPreCalculated, false);
	connect(actionPreCalculated, &QAction::triggered, this, &iAGEMSeModuleInterface::loadPreCalculatedData);
}

void iAGEMSeModuleInterface::startGEMSe()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
	AttachToMdiChild(m_mdiChild);
}

iAModuleAttachmentToChild* iAGEMSeModuleInterface::CreateAttachment(MainWindow* mainWnd, MdiChild * child)
{
	iAGEMSeAttachment* result = iAGEMSeAttachment::create( mainWnd, child);
	if (result)
	{
		setupToolbar();
	}
	return result;
}

void iAGEMSeModuleInterface::loadPreCalculatedData()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Load Precalculated Sampling & Clustering Data"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->filePath() : QString(),
		tr("GEMSe project (*.sea );;") );
	if (fileName.isEmpty())
		return;
	loadOldGEMSeProject(fileName);
}

void iAGEMSeModuleInterface::loadOldGEMSeProject(QString const & fileName)
{
	if (m_seaFile)
	{
		DEBUG_LOG("A loading procedure is currently in progress. Please let this finish first.");
		return;
	}
	m_seaFile = QSharedPointer<iASEAFile>(new iASEAFile(fileName));
	if (!m_seaFile->good())
	{
		DEBUG_LOG(QString("GEMSe data %1 file could not be read.").arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
	m_mdiChild = m_mainWnd->createMdiChild(false);
	connect(m_mdiChild, &MdiChild::fileLoaded, this, &iAGEMSeModuleInterface::loadGEMSe);
	if (!m_mdiChild->loadFile(m_seaFile->modalityFileName(), false))
	{
		DEBUG_LOG(QString("Failed to load project '%1' referenced from precalculated GEMSe data file %2.")
			.arg(m_seaFile->modalityFileName())
			.arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
}

void iAGEMSeModuleInterface::loadProject(MdiChild* mdiChild, QSettings const & metaFile, QString const & fileName)
{
	m_mdiChild = mdiChild;
	m_seaFile = QSharedPointer<iASEAFile>(new iASEAFile(metaFile, fileName));
	loadGEMSe();
}

void iAGEMSeModuleInterface::saveProject(QSettings & metaFile, QString const & fileName)
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("Could not store project - no GEMSE module attached to current child!");
		return;
	}
	gemseAttach->saveProject(metaFile, fileName);
}

void iAGEMSeModuleInterface::loadGEMSe()
{
	if (!m_seaFile->good())
	{
		DEBUG_LOG(QString("GEMSe data in file '%1' could not be read.").arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
	// load segmentation explorer:
	bool result = AttachToMdiChild( m_mdiChild );
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!result || !gemseAttach)
	{
		DEBUG_LOG("GEMSE attachment could not be created!");
		m_seaFile.clear();
		return;
	}
	// load sampling data:
	QMap<int, QString> const & samplings = m_seaFile->samplings();
	for (int key : samplings.keys())
	{
		result &= gemseAttach->loadSampling(samplings[key], m_seaFile->labelCount(), key);
		if (!result)
			break;
	}
	if (!result || !gemseAttach->loadClustering(m_seaFile->clusteringFileName()))
	{
		DEBUG_LOG(QString("Loading precomputed GEMSe data from file %1 failed!").arg(m_seaFile->fileName()));
	}
	if (m_seaFile->layoutName() != "")
	{
		m_mdiChild->loadLayout(m_seaFile->layoutName());
	}
	if (m_seaFile->referenceImage() != "")
	{
		gemseAttach->loadRefImg(m_seaFile->referenceImage());
	}
	if (m_seaFile->hiddenCharts() != "")
	{
		gemseAttach->setSerializedHiddenCharts(m_seaFile->hiddenCharts());
	}
	gemseAttach->setLabelInfo(m_seaFile->colorTheme(), m_seaFile->labelNames());
	m_seaFile.clear();
}

void iAGEMSeModuleInterface::setupToolbar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAGEMSeToolbar("GEMSe ToolBar", m_mainWnd);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);

	connect(m_toolbar->action_ResetFilter, &QAction::triggered, this, &iAGEMSeModuleInterface::resetFilter);
	connect(m_toolbar->action_ToggleAutoShrink, &QAction::triggered, this, &iAGEMSeModuleInterface::toggleAutoShrink);
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, &iAGEMSeModuleInterface::toggleDockWidgetTitleBar);
	connect(m_toolbar->action_ExportIDs, &QAction::triggered, this, &iAGEMSeModuleInterface::exportClusterIDs);
	connect(m_toolbar->action_ExportAttributeRangeRanking, &QAction::triggered, this, &iAGEMSeModuleInterface::exportAttributeRangeRanking);
	connect(m_toolbar->action_ExportRanking, &QAction::triggered, this, &iAGEMSeModuleInterface::exportRankings);
	connect(m_toolbar->action_ImportRanking, &QAction::triggered, this, &iAGEMSeModuleInterface::importRankings);
}

void iAGEMSeModuleInterface::resetFilter()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->resetFilter();
}

void iAGEMSeModuleInterface::toggleAutoShrink()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->toggleAutoShrink();
}

void iAGEMSeModuleInterface::toggleDockWidgetTitleBar()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->toggleDockWidgetTitleBar();
}

void iAGEMSeModuleInterface::exportClusterIDs()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->exportClusterIDs();
}

void iAGEMSeModuleInterface::exportAttributeRangeRanking()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->exportAttributeRangeRanking();
}


void iAGEMSeModuleInterface::exportRankings()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->exportRankings();
}


void iAGEMSeModuleInterface::importRankings()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->importRankings();
}

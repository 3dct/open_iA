/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iASEAFile.h"

#include <dlg_modalities.h>
#include <iAConsole.h>
#include <iAModality.h>
#include <iAModuleDispatcher.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <QFileDialog>
#include <QList>
#include <QSettings>
#include <QTextDocument>

#include <cassert>

iAGEMSeModuleInterface::iAGEMSeModuleInterface():
	m_toolbar(0)
{}

void iAGEMSeModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuEnsembles = getMenuWithTitle( toolsMenu, tr( "Image Ensembles" ), false );
	
	QAction * actionGEMSe = new QAction( tr("GEMSe"), nullptr);
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionGEMSe, true);
	connect(actionGEMSe, SIGNAL(triggered()), this, SLOT(StartGEMSe()));

	QAction * actionPreCalculated = new QAction( tr("Load Segmentation Ensemble in GEMSe"), nullptr );
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionPreCalculated, false);
	connect(actionPreCalculated, SIGNAL(triggered()), this, SLOT(LoadPreCalculatedData()));
}

void iAGEMSeModuleInterface::StartGEMSe()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
	AttachToMdiChild(m_mdiChild);
}

iAModuleAttachmentToChild* iAGEMSeModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	iAGEMSeAttachment* result = iAGEMSeAttachment::create( mainWnd, childData);
	if (result)
	{
		SetupToolbar();
	}
	return result;
}

void iAGEMSeModuleInterface::LoadPreCalculatedData()
{
	QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
		tr("Load Precalculated Sampling & Clustering Data"),
		m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->getFilePath() : QString(),
		tr("GEMSe project (*.sea );;") );
	if (fileName != "")
	{
		iASEAFile seaFile(fileName);
		LoadPreCalculatedData(seaFile);
	}
}

void iAGEMSeModuleInterface::LoadPreCalculatedData(iASEAFile const & seaFile)
{
	m_mdiChild = m_mainWnd->createMdiChild(false);
	if (!seaFile.good())
	{
		DEBUG_LOG(QString("Precalculated GEMSe data %1 file could not be read.").arg(seaFile.GetSEAFileName()));
		return;
	}
	if (m_seaFile)
	{
		DEBUG_LOG("A loading procedure is currently in progress. Please let this finish first.");
		return;
	}
	m_seaFile = QSharedPointer<iASEAFile>(new iASEAFile(seaFile));
	connect(m_mdiChild, SIGNAL(fileLoaded()), this, SLOT(continuePreCalculatedDataLoading()));
	if (!m_mdiChild->loadFile(seaFile.GetModalityFileName(), false))
	{
		DEBUG_LOG(QString("Failed to load project '%1' referenced from precalculated GEMSe data file %2.")
			.arg(seaFile.GetModalityFileName())
			.arg(seaFile.GetSEAFileName()));
		m_seaFile.clear();
		return;
	}
}

void iAGEMSeModuleInterface::continuePreCalculatedDataLoading()
{
	UpdateChildData();

	// load segmentation explorer:
	bool result = AttachToMdiChild( m_mdiChild );
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!result || !gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	// load sampling data:
	QMap<int, QString> const & samplings = m_seaFile->GetSamplings();
	for (int key : samplings.keys())
	{
		result &= gemseAttach->LoadSampling(samplings[key], m_seaFile->GetLabelCount(), key);
		if (!result)
			break;
	}
	if (!result || !gemseAttach->LoadClustering(m_seaFile->GetClusteringFileName()))
	{
		DEBUG_LOG(QString("Loading precomputed GEMSe data from file %1 failed!").arg(m_seaFile->GetSEAFileName()));
	}
	if (m_seaFile->GetLayoutName() != "")
	{
		m_mdiChild->LoadLayout(m_seaFile->GetLayoutName());
	}
	if (m_seaFile->GetReferenceImage() != "")
	{
		gemseAttach->LoadRefImg(m_seaFile->GetReferenceImage());
	}
	if (m_seaFile->GetHiddenCharts() != "")
	{
		gemseAttach->SetSerializedHiddenCharts(m_seaFile->GetHiddenCharts());
	}
	gemseAttach->SetLabelInfo(m_seaFile->GetColorTheme(), m_seaFile->GetLabelNames());
	m_seaFile.clear();
}

void iAGEMSeModuleInterface::SetupToolbar()
{
	if (m_toolbar)
	{
		return;
	}
	m_toolbar = new iAGEMSeToolbar("GEMSe ToolBar", m_mainWnd);
	m_mainWnd->addToolBar(Qt::BottomToolBarArea, m_toolbar);

	connect(m_toolbar->action_ResetFilter, SIGNAL(triggered()), this, SLOT(ResetFilter()));
	connect(m_toolbar->action_ToggleAutoShrink, SIGNAL(triggered()), this, SLOT(ToggleAutoShrink()));
	connect(m_toolbar->action_ToggleTitleBar, SIGNAL(triggered()), this, SLOT(ToggleDockWidgetTitleBar()));
	connect(m_toolbar->action_ExportIDs, SIGNAL(triggered()), this, SLOT(ExportClusterIDs()));
	connect(m_toolbar->action_ExportAttributeRangeRanking, SIGNAL(triggered()), this, SLOT(ExportAttributeRangeRanking()));
	connect(m_toolbar->action_ExportRanking, SIGNAL(triggered()), this, SLOT(ExportRankings()));
	connect(m_toolbar->action_ImportRanking, SIGNAL(triggered()), this, SLOT(ImportRankings()));
}

void iAGEMSeModuleInterface::ResetFilter()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ResetFilter();
}

void iAGEMSeModuleInterface::ToggleAutoShrink()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ToggleAutoShrink();
}

void iAGEMSeModuleInterface::ToggleDockWidgetTitleBar()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ToggleDockWidgetTitleBar();
}

void iAGEMSeModuleInterface::ExportClusterIDs()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ExportClusterIDs();
}

void iAGEMSeModuleInterface::ExportAttributeRangeRanking()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ExportAttributeRangeRanking();
}


void iAGEMSeModuleInterface::ExportRankings()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ExportRankings();
}


void iAGEMSeModuleInterface::ImportRankings()
{
	iAGEMSeAttachment* gemseAttach = GetAttachment<iAGEMSeAttachment>();
	if (!gemseAttach)
	{
		DEBUG_LOG("GEMSE module is not attached!");
		return;
	}
	gemseAttach->ImportRankings();
}

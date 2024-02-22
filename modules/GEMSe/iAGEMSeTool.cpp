// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGEMSeTool.h"

#include "dlg_GEMSeControl.h"
#include "dlg_GEMSe.h"
#include "dlg_samplings.h"
#include "iAGEMSeModuleInterface.h"
#include "iAGEMSeTool.h"
#include "iASEAFile.h"

#include <iAColorTheme.h>
#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h> // TODO: Refactor; it shouldn't be required to go via iAModuleDispatcher to retrieve one's own module

namespace
{
	const QString defaultThemeName("Brewer Set3 (max. 12)");
}

const QString iAGEMSeTool::ID("GEMSe");

iAGEMSeTool::iAGEMSeTool(iAMainWindow* mainWnd, iAMdiChild* child):
	iATool(mainWnd, child),
	m_dummyTitleWidget(new QWidget()),
	m_dlgSamplings(new dlg_samplings()),
	m_dlgGEMSe(new dlg_GEMSe(child, iAColorThemeManager::instance().theme(defaultThemeName))),
	m_dlgGEMSeControl(new dlg_GEMSeControl(child, m_dlgGEMSe, m_dlgSamplings,
		iAColorThemeManager::instance().theme(defaultThemeName)))
{
	child->splitDockWidget(child->renderDockWidget(), m_dlgGEMSe, Qt::Vertical);
	child->splitDockWidget(child->renderDockWidget(), m_dlgGEMSeControl, Qt::Horizontal);
	child->splitDockWidget(m_dlgGEMSeControl, m_dlgSamplings, Qt::Vertical);
}

void iAGEMSeTool::loadState(QSettings & projectFile, QString const & fileName)
{
	auto gemseModule = m_mainWindow->moduleDispatcher().module<iAGEMSeModuleInterface>();
	gemseModule->setupToolbar();

	auto seaFile = std::make_shared<iASEAFile>(projectFile, fileName);

	if (!seaFile->good())
	{
		LOG(lvlError, QString("GEMSe data in file '%1' could not be read.").arg(seaFile->fileName()));
		seaFile.reset();
		return;
	}
	// load sampling data:
	bool result = true;
	QMap<int, QString> const& samplings = seaFile->samplings();
	for (int key : samplings.keys())
	{
		result &= loadSampling(samplings[key], seaFile->labelCount(), key);
		if (!result)
			break;
	}
	if (!result || !loadClustering(seaFile->clusteringFileName()))
	{
		LOG(lvlError, QString("Loading precomputed GEMSe data from file %1 failed!").arg(seaFile->fileName()));
	}
	if (seaFile->layoutName() != "")
	{
		m_child->loadLayout(seaFile->layoutName());
	}
	if (seaFile->referenceImage() != "")
	{
		loadRefImg(seaFile->referenceImage());
	}
	if (seaFile->hiddenCharts() != "")
	{
		setSerializedHiddenCharts(seaFile->hiddenCharts());
	}
	setLabelInfo(seaFile->colorTheme(), seaFile->labelNames());
}

void iAGEMSeTool::saveState(QSettings & projectFile, QString const & fileName)
{
	m_dlgGEMSeControl->saveProject(projectFile, fileName);
}

std::shared_ptr<iATool> iAGEMSeTool::create(iAMainWindow* mainWnd, iAMdiChild* child)
{
	return std::make_shared<iAGEMSeTool>(mainWnd, child);;
}

bool iAGEMSeTool::loadSampling(QString const& smpFileName, int labelCount, int datasetID)
{
	return m_dlgGEMSeControl->loadSampling(smpFileName, labelCount, datasetID);
}

bool iAGEMSeTool::loadClustering(QString const& fileName)
{
	return m_dlgGEMSeControl->loadClustering(fileName);
}

bool iAGEMSeTool::loadRefImg(QString const& refImgName)
{
	return m_dlgGEMSeControl->loadRefImg(refImgName);
}

void iAGEMSeTool::setSerializedHiddenCharts(QString const& hiddenCharts)
{
	return m_dlgGEMSeControl->setSerializedHiddenCharts(hiddenCharts);
}

void iAGEMSeTool::resetFilter()
{
	m_dlgGEMSe->ResetFilters();
}

void iAGEMSeTool::toggleAutoShrink()
{
	m_dlgGEMSe->ToggleAutoShrink();
}

void iAGEMSeTool::toggleDockWidgetTitleBar()
{
	QWidget* titleBar = m_dlgGEMSe->titleBarWidget();
	if (titleBar == m_dummyTitleWidget)
	{
		m_dlgGEMSe->setTitleBarWidget(nullptr);
	}
	else
	{
		m_dlgGEMSe->setTitleBarWidget(m_dummyTitleWidget);
	}
}

void iAGEMSeTool::exportClusterIDs()
{
	m_dlgGEMSeControl->exportIDs();
}

void iAGEMSeTool::exportAttributeRangeRanking()
{
	m_dlgGEMSeControl->exportAttributeRangeRanking();
}

void iAGEMSeTool::exportRankings()
{
	m_dlgGEMSeControl->exportRankings();
}

void iAGEMSeTool::importRankings()
{
	m_dlgGEMSeControl->importRankings();
}

void iAGEMSeTool::setLabelInfo(QString const& colorTheme, QString const& labelNames)
{
	m_dlgGEMSeControl->setLabelInfo(colorTheme, labelNames);
}

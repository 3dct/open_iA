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
#include "iAGEMSeModuleInterface.h"

#include "iAGEMSeTool.h"
#include "iARepresentative.h"
#include "iASEAFile.h"

#include <iADataSet.h>
#include <iALog.h>
#include <iAFilterDefault.h>
#include <iAModality.h>
#include <iAToolRegistry.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>

#include <cassert>

IAFILTER_DEFAULT_CLASS(iADifferenceMarker);

iADifferenceMarker::iADifferenceMarker():
	iAFilter("Difference marker", "Intensity", "Computes an image where differences are marked with the given marker value.<br/>"
		"The filter is meant for labelled images; both input images are required to have the same data type. "
		"It also works for any other voxel data types, but for "
		"<em>Difference marker value</em> specifies the intensity value that should be used to mark regions"
		"where the two given images deviate. Where the images are the same, "
		"this same value will be used in the output image as well.", 2)
{
	addParameter("Difference marker value", iAValueType::Continuous);
	setInputName(1u, "Difference to");
}

void iADifferenceMarker::performWork(QVariantMap const & params)
{
	QVector<iAITKIO::ImagePointer> imgs;
	imgs.push_back(imageInput(0)->itkImage());
	imgs.push_back(imageInput(1)->itkImage());
	auto out = CalculateDifferenceMarkers(imgs, params["Difference marker value"].toDouble());
	if (!out)
	{
		addMsg("No output generated, check additional messages in debug log.");
	}
	else
	{
		addOutput(out);
	}
}

iAGEMSeModuleInterface::iAGEMSeModuleInterface():
	m_toolbar(nullptr)
{}

void iAGEMSeModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	Q_INIT_RESOURCE(GEMSe);

	iAToolRegistry::addTool(iAGEMSeTool::ID, iAGEMSeTool::create);

	QAction * actionGEMSe = new QAction(tr("GEMSe"), m_mainWnd);
	m_mainWnd->makeActionChildDependent(actionGEMSe);
	connect(actionGEMSe, &QAction::triggered, this, [this]()
		{
			addToolToActiveMdiChild<iAGEMSeTool>("GEMSe", m_mainWnd);
			setupToolbar();
		});

	QAction * actionPreCalculated = new QAction(tr("GEMSe - Load Ensemble (old)"), m_mainWnd);
	connect(actionPreCalculated, &QAction::triggered, this, [this]()
		{
			QString fileName = QFileDialog::getOpenFileName(m_mainWnd,
				tr("Load Precalculated Sampling & Clustering Data"),
				m_mainWnd->activeMdiChild() ? m_mainWnd->activeMdiChild()->filePath() : QString(),
				tr("GEMSe project (*.sea );;All files (*)"));
			if (fileName.isEmpty())
			{
				return;
			}
			loadOldGEMSeProject(fileName);
		});

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionGEMSe);
	submenu->addAction(actionPreCalculated);
}

void iAGEMSeModuleInterface::loadOldGEMSeProject(QString const & fileName)
{
	if (m_seaFile)
	{
		LOG(lvlWarn, "A loading procedure is currently in progress. Please let this finish first.");
		return;
	}
	m_seaFile = QSharedPointer<iASEAFile>::create(fileName);
	if (!m_seaFile->good())
	{
		LOG(lvlError, QString("GEMSe data %1 file could not be read.").arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
	m_mdiChild = m_mainWnd->createMdiChild(false);
	connect(m_mdiChild, &iAMdiChild::fileLoaded, this, &iAGEMSeModuleInterface::loadGEMSe);
	if (!m_mdiChild->loadFile(m_seaFile->modalityFileName(), false))
	{
		LOG(lvlError, QString("Failed to load project '%1' referenced from precalculated GEMSe data file %2.")
			.arg(m_seaFile->modalityFileName())
			.arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
}

void iAGEMSeModuleInterface::loadProject(iAMdiChild* mdiChild, QSettings const & metaFile, QString const & fileName)
{
	m_mdiChild = mdiChild;
	m_seaFile = QSharedPointer<iASEAFile>::create(metaFile, fileName);
	loadGEMSe();
}

void iAGEMSeModuleInterface::saveProject(QSettings & metaFile, QString const & fileName)
{
	auto t = getTool<iAGEMSeTool>(m_mdiChild);
	if (!t)
	{
		LOG(lvlError, "Could not store project - no GEMSE tool attached to current child!");
		return;
	}
	t->saveProject(metaFile, fileName);
}

void iAGEMSeModuleInterface::loadGEMSe()
{
	if (!m_seaFile->good())
	{
		LOG(lvlError, QString("GEMSe data in file '%1' could not be read.").arg(m_seaFile->fileName()));
		m_seaFile.clear();
		return;
	}
	// load segmentation explorer:
	auto t = addToolToActiveMdiChild<iAGEMSeTool>("GEMSe", m_mainWnd);
	setupToolbar();
	if (!t)
	{
		LOG(lvlError, "GEMSE tool could not be created!");
		m_seaFile.clear();
		return;
	}
	// load sampling data:
	bool result = true;
	QMap<int, QString> const & samplings = m_seaFile->samplings();
	for (int key : samplings.keys())
	{
		result &= t->loadSampling(samplings[key], m_seaFile->labelCount(), key);
		if (!result)
			break;
	}
	if (!result || !t->loadClustering(m_seaFile->clusteringFileName()))
	{
		LOG(lvlError, QString("Loading precomputed GEMSe data from file %1 failed!").arg(m_seaFile->fileName()));
	}
	if (m_seaFile->layoutName() != "")
	{
		m_mdiChild->loadLayout(m_seaFile->layoutName());
	}
	if (m_seaFile->referenceImage() != "")
	{
		t->loadRefImg(m_seaFile->referenceImage());
	}
	if (m_seaFile->hiddenCharts() != "")
	{
		t->setSerializedHiddenCharts(m_seaFile->hiddenCharts());
	}
	t->setLabelInfo(m_seaFile->colorTheme(), m_seaFile->labelNames());
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

	auto toolbarCallback = [this](auto thisfunc) {
		auto t = getTool<iAGEMSeTool>(m_mdiChild);
		if (!t)
		{
			LOG(lvlError, "ERROR: GEMSE tool is not available!");
			return;
		}
		std::invoke(thisfunc, t);
	};

	connect(m_toolbar->action_ResetFilter, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::resetFilter);
	});
	connect(m_toolbar->action_ToggleAutoShrink, &QAction::triggered, this,[this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::toggleAutoShrink);
	});
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::toggleDockWidgetTitleBar);
	});
	connect(m_toolbar->action_ExportIDs, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::exportClusterIDs);
	});
	connect(m_toolbar->action_ExportAttributeRangeRanking, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::exportAttributeRangeRanking);
	});
	connect(m_toolbar->action_ExportRanking, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::exportRankings);
	});
	connect(m_toolbar->action_ImportRanking, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::importRankings);
	});
}

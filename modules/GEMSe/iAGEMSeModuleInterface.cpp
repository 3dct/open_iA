// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGEMSeModuleInterface.h"

#include "iAGEMSeTool.h"
#include "iARepresentative.h"

#include <iADataSet.h>
#include <iALog.h>
#include <iAFilterDefault.h>
#include <iAToolRegistry.h>
#include <iAToolHelper.h>    // for addToolToActiveMdiChild

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

	QMenu* submenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Image Ensembles"), true);
	submenu->addAction(actionGEMSe);
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
		(t->*thisfunc)();
		//std::invoke(thisfunc, t);    // use once we have switched to C++17
	};
	connect(m_toolbar->action_ResetFilter, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::resetFilter);
	});
	connect(m_toolbar->action_ToggleAutoShrink, &QAction::triggered, this,[this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::toggleAutoShrink);
	});
	m_mainWnd->addActionIcon(m_toolbar->action_ToggleTitleBar, "titlebar-off");
	connect(m_toolbar->action_ToggleTitleBar, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::toggleDockWidgetTitleBar);
	});
	m_mainWnd->addActionIcon(m_toolbar->action_ExportIDs, "export_ids");
	connect(m_toolbar->action_ExportIDs, &QAction::triggered, this, [this, toolbarCallback]() {
		toolbarCallback(&iAGEMSeTool::exportClusterIDs);
	});
	m_mainWnd->addActionIcon(m_toolbar->action_ExportAttributeRangeRanking, "export_ranges");
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

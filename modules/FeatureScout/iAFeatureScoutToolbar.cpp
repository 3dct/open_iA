// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFeatureScoutToolbar.h"

#include "dlg_FeatureScout.h"
#include "iAFeatureScoutTool.h"

#include <iALog.h>

#include "iAMainWindow.h"
#include "iAMdiChild.h"

#include "ui_FeatureScoutToolBar.h"

iAFeatureScoutToolbar* iAFeatureScoutToolbar::tlbFeatureScout{};

void iAFeatureScoutToolbar::addForChild(iAMainWindow* mainWnd, iAMdiChild* child)
{
	if (!tlbFeatureScout)
	{
		tlbFeatureScout = new iAFeatureScoutToolbar(mainWnd);
		mainWnd->addToolBar(Qt::BottomToolBarArea, tlbFeatureScout);
		tlbFeatureScout->setVisible(true);
	}
	else    // make sure toolbar is enabled when FeatureScout has finished loading
	{       // for all FeatureScout instances after the first:
		tlbFeatureScout->setEnabled(true);
	}
	connect(child, &iAMdiChild::closed, tlbFeatureScout, &iAFeatureScoutToolbar::childClosed);
	connect(child, &iAMdiChild::toolRemoved, [] (QString const& key)
		{
			if (key == iAFeatureScoutTool::ID)
			{
				iAFeatureScoutToolbar::tlbFeatureScout->childClosed(nullptr);
			}
		});
}

iAFeatureScoutToolbar::iAFeatureScoutToolbar(iAMainWindow* mainWnd) :
	QToolBar("FeatureScout ToolBar", mainWnd),
	m_ui(std::make_unique<Ui_FeatureScoutToolBar>()),
	m_mainWnd(mainWnd)
{
	m_ui->setupUi(this);
	auto toolbarCallback = [this](auto thisfunc)
	{
		auto fs = getTool<iAFeatureScoutTool>(m_mainWnd->activeMdiChild());
		if (!fs)
		{
			LOG(lvlInfo, "No FeatureScout tool open in current iAMdiChild!");
			return;
		}
		std::invoke(thisfunc, fs->featureScout());
	};
	connect(m_mainWnd, &iAMainWindow::childChanged, this, &iAFeatureScoutToolbar::childChanged);
	m_mainWnd->addActionIcon(m_ui->actionMultiRendering, "layers");
	m_mainWnd->addActionIcon(m_ui->actionLength_Distribution, "length-distribution");
	m_mainWnd->addActionIcon(m_ui->actionMeanObject, "mean-object");
	m_mainWnd->addActionIcon(m_ui->actionOrientation_Rendering, "compass");
	m_mainWnd->addActionIcon(m_ui->actionActivate_SPM, "SPM");
	m_mainWnd->addActionIcon(m_ui->actionSettingsPC, "settings_PC");
	m_mainWnd->addActionIcon(m_ui->actionSaveMesh, "save");
	connect(m_ui->actionLength_Distribution, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::renderLengthDistribution); });
	connect(m_ui->actionMeanObject, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::renderMeanObject); });
	connect(m_ui->actionMultiRendering, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::multiClassRendering); });
	connect(m_ui->actionOrientation_Rendering, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::renderOrientation); });
	connect(m_ui->actionActivate_SPM, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::showScatterPlot); });
	connect(m_ui->actionSettingsPC, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::showPCSettings); });
	connect(m_ui->actionSaveMesh, &QAction::triggered, this,
		[toolbarCallback]() { toolbarCallback(&dlg_FeatureScout::saveMesh);  });
}

iAFeatureScoutToolbar::~iAFeatureScoutToolbar() = default;

void iAFeatureScoutToolbar::childClosed(iAMdiChild* closingChild)
{
	for (auto mdiChild: m_mainWnd->mdiChildList())
	{
		if (mdiChild != closingChild && getTool<iAFeatureScoutTool>(mdiChild))
		{	// if there's at least one current child with a FeatureScout widget, keep toolbar
			return;
		}
	}
	m_mainWnd->removeToolBar(this);
	tlbFeatureScout = nullptr;
	deleteLater();
}

void iAFeatureScoutToolbar::childChanged()
{
	setEnabled(getTool<iAFeatureScoutTool>(m_mainWnd->activeMdiChild()));
}

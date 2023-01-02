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
#include "iAFeatureScoutToolbar.h"

#include <iALog.h>

#include "iAMainWindow.h"
#include "iAMdiChild.h"

#include "dlg_FeatureScout.h"

#include "ui_FeatureScoutToolBar.h"

iAFeatureScoutToolbar* iAFeatureScoutToolbar::tlbFeatureScout{};

namespace
{
	dlg_FeatureScout* getFSFromChild(iAMdiChild* child)
	{
		if (!child)
		{
			return nullptr;
		}
		return child->findChild<dlg_FeatureScout*>(dlg_FeatureScout::DlgObjectName);
	}
}

void iAFeatureScoutToolbar::addForChild(iAMainWindow* mainWnd, iAMdiChild* child)
{
	if (!getFSFromChild(child))
	{
		LOG(lvlWarn, "Tried to add FeatureScout toolbar for child which does not have an open FeatureScout instance!");
		return;
	}
	if (!tlbFeatureScout)
	{
		tlbFeatureScout = new iAFeatureScoutToolbar(mainWnd);
		mainWnd->addToolBar(Qt::BottomToolBarArea, tlbFeatureScout);
		tlbFeatureScout->setVisible(true);
	}
	else    // make sure toolbar is enabled when FeatureScout has finished loading
	{       // for all FeatureScout instances after the first:
		tlbFeatureScout->childChanged();
	}
	connect(child, &iAMdiChild::closed, tlbFeatureScout, &iAFeatureScoutToolbar::childClosed);
}

iAFeatureScoutToolbar::iAFeatureScoutToolbar(iAMainWindow* mainWnd) :
	QToolBar("FeatureScout ToolBar", mainWnd),
	m_ui(std::make_unique<Ui_FeatureScoutToolBar>()),
	m_mainWnd(mainWnd)
{
	m_ui->setupUi(this);
	auto toolbarCallback = [this](auto thisfunc) {
		auto fs = getFSFromChild(m_mainWnd->activeMdiChild());
		if (!fs)
		{
			LOG(lvlInfo, "No FeatureScout tool open in current iAMdiChild!");
			return;
		}
		std::invoke(thisfunc, fs);
	};
	connect(m_mainWnd, &iAMainWindow::childChanged, this, &iAFeatureScoutToolbar::childChanged);
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
}

iAFeatureScoutToolbar::~iAFeatureScoutToolbar() = default;

void iAFeatureScoutToolbar::childClosed()
{
	for (auto mdiChild: m_mainWnd->mdiChildList())
	{
		if (getFSFromChild(mdiChild))
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
	setEnabled(getFSFromChild(m_mainWnd->activeMdiChild()));
}

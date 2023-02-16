// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFuzzyFeatureTrackingModuleInterface.h"

#include "iAFuzzyFeatureTrackingTool.h"

#include <iAToolHelper.h>    // for addToolToActiveMdiChild

#include <QAction>
#include <QMenu>
#include <QMessageBox>

void iAFuzzyFeatureTrackingModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QAction * actionFuzzyFeatureTracking = new QAction(tr("Fuzzy Feature Tracking"), m_mainWnd);
	connect(actionFuzzyFeatureTracking, &QAction::triggered, this, &iAFuzzyFeatureTrackingModuleInterface::fuzzyFeatureTracking);
	m_mainWnd->makeActionChildDependent(actionFuzzyFeatureTracking);
	QMenu* featureAnalysisMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("Feature Analysis"), true);
	featureAnalysisMenu->addAction(actionFuzzyFeatureTracking);
}

void iAFuzzyFeatureTrackingModuleInterface::fuzzyFeatureTracking()
{
	try
	{
		addToolToActiveMdiChild<iAFuzzyFeatureTrackingTool>("FuzzyFeatureTracking", m_mainWnd);
	}
	catch (std::exception& e)
	{
		QString msg = QString("Failed to create Fuzzy Feature Tracking Tool: %1. Please fix the error, close the current window, and then restart the analysis!").arg(e.what());
		QMessageBox::warning(m_mainWnd, "Fuzzy Feature Tracking", msg);
		LOG(lvlError, msg);
	}
}

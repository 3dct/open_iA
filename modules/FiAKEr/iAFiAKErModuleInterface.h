// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_FiAKErToolBar.h"

#include "iAGUIModuleInterface.h"
#include "qthelper/iAQTtoUIConnector.h"

class iAFiAKErTool;

class QSettings;

typedef iAQTtoUIConnector<QToolBar, Ui_FiAKErToolBar> iAFiAKErToolBar;

class iAFiAKErModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
	void SaveSettings() const override;

	void setupToolBar();  // TODO: move to separate class, like iAFeatureScoutToolBar (maybe a reusable functionality?)

private slots:
	void startFiAKEr();

	//! Method to load fiaker project (called on Tools->FIAKER->Load project)
	//! @deprecated use open_iA project feature instead!
	void loadFiAKErProject();

	void toggleDockWidgetTitleBars();
	void toggleSettings();
private:
	iAFiAKErToolBar* m_toolbar = nullptr;
	QString m_lastPath, m_lastFormat;
	double m_lastTimeStepOffset;
	bool m_lastUseStepData, m_lastShowPreviews, m_lastShowCharts;
};

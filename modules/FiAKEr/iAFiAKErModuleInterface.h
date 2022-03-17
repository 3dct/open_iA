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
#pragma once

#include "ui_FiAKErToolBar.h"

#include "iAGUIModuleInterface.h"
#include "qthelper/iAQTtoUIConnector.h"

class iAFIAKERProject;

class QSettings;

typedef iAQTtoUIConnector<QToolBar, Ui_FiAKErToolBar> iAFiAKErToolBar;

class iAFiAKErModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	void Initialize() override;
	void SaveSettings() const override;

	void setupToolBar();
	void loadProject(iAMdiChild* mdiChild, QSettings const& projectFile, QString const& fileName, iAFIAKERProject* project);
protected:
	iAModuleAttachmentToChild* CreateAttachment(iAMainWindow* mainWnd, iAMdiChild* child) override;
private slots:
	void startFiAKEr();

	//! Method to load fiaker project (called on Tools->FIAKER->Load project)
	//! Deprecated, use open_iA project feature instead!
	void loadFiAKErProject();

	void toggleDockWidgetTitleBars();
	void toggleSettings();
private:
	iAFiAKErToolBar* m_toolbar = nullptr;
	QString m_lastPath, m_lastFormat;
	double m_lastTimeStepOffset;
	bool m_lastUseStepData, m_lastShowPreviews, m_lastShowCharts;
};

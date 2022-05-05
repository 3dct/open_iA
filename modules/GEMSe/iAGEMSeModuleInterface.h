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

#include "ui_GEMSeToolBar.h"

#include <iAGUIModuleInterface.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <QToolBar>

class iASEAFile;

class QSettings;

typedef iAQTtoUIConnector<QToolBar, Ui_GEMSeToolBar> iAGEMSeToolbar;

class iAGEMSeModuleInterface : public iAGUIModuleInterface
{
	Q_OBJECT
public:
	iAGEMSeModuleInterface();
	void Initialize() override;
	void loadProject(iAMdiChild* mdiChild, QSettings const & metaFile, QString const & fileName);
	void saveProject(QSettings & metaFile, QString const & fileName);
protected:
	iAModuleAttachmentToChild* CreateAttachment(iAMainWindow* mainWnd, iAMdiChild * child) override;
private slots:
	//! @{ Menu entries:
	void startGEMSe();
	void loadPreCalculatedData();
	//! @}
	//! @{ Toolbar actions:
	void resetFilter();
	void toggleAutoShrink();
	void toggleDockWidgetTitleBar();
	void exportClusterIDs();
	void exportAttributeRangeRanking();
	void exportRankings();
	void importRankings();
	//! @}
	void loadGEMSe();
private:
	void loadOldGEMSeProject(QString const & fileName);
	void setupToolbar();

	iAGEMSeToolbar* m_toolbar;

	//! cache for precalculated data loading
	QSharedPointer<iASEAFile> m_seaFile;
};

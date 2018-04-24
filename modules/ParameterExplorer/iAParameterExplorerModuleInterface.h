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
#pragma once

#include "iAModuleInterface.h"
#include "ui_ParamToolBar.h"
#include "iAQTtoUIConnector.h"

#include <QToolBar>

typedef iAQTtoUIConnector<QToolBar, Ui_ParamToolBar> iAParamToolBar;

class iAParameterExplorerModuleInterface : public iAModuleInterface
{
	Q_OBJECT
public:
	void Initialize();
protected:
	iAModuleAttachmentToChild* CreateAttachment(MainWindow* mainWnd, iAChildData childData) override;
private slots:
	void StartParameterExplorer();
	void ToggleDockWidgetTitleBars();
	void ToggleSettings();
	void SaveState();
	void LoadState();
	void ContinueStateLoading();
private:
	bool CreateAttachment(QString const & csvFileName, MdiChild* child);
private:
	void SetupToolBar();
	iAParamToolBar * m_toolBar;
	QMap<MdiChild*, QString> m_stateFiles;
};

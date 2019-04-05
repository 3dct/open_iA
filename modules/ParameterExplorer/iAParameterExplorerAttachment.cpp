/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAParameterExplorerAttachment.h"

#include "iAParamSPLOMView.h"
#include "iAParamSpatialView.h"
#include "iAParamTableView.h"

#include <mdichild.h>
#include <qthelper/iADockWidgetWrapper.h>

#include <QFileDialog>

iAParameterExplorerAttachment* iAParameterExplorerAttachment::create(MainWindow * mainWnd, MdiChild * child)
{
	return new iAParameterExplorerAttachment(mainWnd, child);
}

iAParameterExplorerAttachment::iAParameterExplorerAttachment(MainWindow * mainWnd, MdiChild * child)
	:iAModuleAttachmentToChild(mainWnd, child)
{
	QString csvFileName = QFileDialog::getOpenFileName(child,
			tr( "Select CSV File" ), child->filePath(), tr( "CSV Files (*.csv);;" ) );
	if (csvFileName.isEmpty())
		return;
	m_tableView = new iAParamTableView(csvFileName);
	m_spatialView = new iAParamSpatialView(m_tableView, QFileInfo(csvFileName).absolutePath());
	m_SPLOMView = new iAParamSPLOMView(m_tableView, m_spatialView);
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial View", "ParamSpatialView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_SPLOMView, "Scatter Plot Matrix View", "ParamSPLOMView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_tableView, "Table View", "ParamTableView"));
	child->splitDockWidget(child->logDockWidget(), m_dockWidgets[0], Qt::Horizontal);
	child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[1], Qt::Horizontal);
	child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[2], Qt::Vertical);
}

void iAParameterExplorerAttachment::ToggleDockWidgetTitleBars()
{
	for (int i = 0; i < m_dockWidgets.size(); ++i)
	{
		m_dockWidgets[i]->toggleTitleBar();
	}
}

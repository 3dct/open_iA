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

// IMPAsSE 	  IMage PArameter Space Explorer

#include "iAParamFeaturesView.h"
#include "iAParamSPLOMView.h"
#include "iAParamSpatialView.h"
#include "iAParamTableView.h"

#include <mdichild.h>
#include <qthelper/iADockWidgetWrapper.h>

iAParameterExplorerAttachment* iAParameterExplorerAttachment::create(MainWindow * mainWnd, iAChildData childData)
{
	return new iAParameterExplorerAttachment(mainWnd, childData);
}

iAParameterExplorerAttachment::iAParameterExplorerAttachment(MainWindow * mainWnd, iAChildData childData)
	:iAModuleAttachmentToChild(mainWnd, childData)
{
}

void iAParameterExplorerAttachment::LoadCSV(QString const & csvFileName)
{
	if (csvFileName.isEmpty())
		return;
	m_csvFileName = csvFileName;
	m_tableView = new iAParamTableView(csvFileName);
	m_spatialView = new iAParamSpatialView(m_tableView, QFileInfo(csvFileName).absolutePath(),
		m_childData.child->getHistogram(), m_childData.child->GetPreferences().HistogramBins);
	m_SPLOMView = new iAParamSPLOMView(m_tableView, m_spatialView);
	m_featuresView = new iAParamFeaturesView(m_tableView->Table());
	connect(m_featuresView, SIGNAL(ShowFeature(int, bool)), m_SPLOMView, SLOT(ShowFeature(int, bool)));
	connect(m_featuresView, SIGNAL(ShowFeature(int, bool)), m_tableView, SLOT(ShowFeature(int, bool)));
	connect(m_featuresView, SIGNAL(InvertFeature(int, bool)), m_SPLOMView, SLOT(InvertFeature(int, bool)));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_spatialView, "Spatial", "ParamSpatialView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_SPLOMView, "Scatter Plot Matrix", "ParamSPLOMView"));
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_tableView, "Table", "ParamTableView"));
	m_dockWidgets.push_back(m_childData.child->getHistogramDockWidget());
	m_dockWidgets.push_back(new iADockWidgetWrapper(m_featuresView, "Features", "ParamFeaturesView"));
	m_childData.child->splitDockWidget(m_childData.child->logs, m_dockWidgets[0], Qt::Horizontal);
	m_childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[1], Qt::Horizontal);
	m_childData.child->splitDockWidget(m_dockWidgets[0], m_dockWidgets[2], Qt::Vertical);
	m_childData.child->splitDockWidget(m_dockWidgets[2], m_dockWidgets[4], Qt::Vertical);
}

void iAParameterExplorerAttachment::ToggleDockWidgetTitleBars()
{
	for (int i = 0; i < m_dockWidgets.size(); ++i)
	{
		m_dockWidgets[i]->toggleTitleBar();
	}
}

void iAParameterExplorerAttachment::ToggleSettings(bool visible)
{
	m_spatialView->ToggleSettings(visible);
	m_SPLOMView->ToggleSettings(visible);
}

QString const & iAParameterExplorerAttachment::CSVFileName() const
{
	return m_csvFileName;
}

void iAParameterExplorerAttachment::SaveSettings(QSettings & settings)
{
	m_featuresView->SaveSettings(settings);
	m_SPLOMView->SaveSettings(settings);
}

void iAParameterExplorerAttachment::LoadSettings(QSettings const & settings)
{
	m_featuresView->LoadSettings(settings);
	m_SPLOMView->LoadSettings(settings);
}

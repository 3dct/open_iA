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
#include "iAGEMSeAttachment.h"

#include "dlg_GEMSeControl.h"
#include "dlg_GEMSe.h"
#include "dlg_samplings.h"

#include <dlg_modalities.h>
#include <iAConsole.h>
#include <iAColorTheme.h>
#include <iALogger.h>
#include <iAModality.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <mdichild.h>
#include <mainwindow.h>
#include <qthelper/iAWidgetAddHelper.h>

iAGEMSeAttachment::iAGEMSeAttachment(MainWindow * mainWnd, MdiChild * child):
	iAModuleAttachmentToChild(mainWnd, child),
	m_dummyTitleWidget(new QWidget())
{
}

iAGEMSeAttachment* iAGEMSeAttachment::create(MainWindow * mainWnd, MdiChild * child)
{
	iAGEMSeAttachment * newAttachment = new iAGEMSeAttachment(mainWnd, child);

	newAttachment->m_widgetAddHelper = QSharedPointer<iAWidgetAddHelper>(new iAWidgetAddHelper(child, child->logDockWidget()));
	
	QString defaultThemeName("Brewer Set3 (max. 12)");
	iAColorTheme const * colorTheme = iAColorThemeManager::instance().theme(defaultThemeName);
	
	newAttachment->m_dlgGEMSe = new dlg_GEMSe(child, child->logger(), colorTheme);
	newAttachment->m_dlgSamplings = new dlg_samplings();
	newAttachment->m_dlgGEMSeControl = new dlg_GEMSeControl(
		child,
		newAttachment->m_dlgGEMSe,
		child->modalitiesDockWidget(),
		newAttachment->m_dlgSamplings,
		colorTheme
	);
	child->splitDockWidget(child->logDockWidget(), newAttachment->m_dlgGEMSe, Qt::Vertical);
	child->splitDockWidget(child->logDockWidget(), newAttachment->m_dlgGEMSeControl, Qt::Horizontal);
	child->splitDockWidget(newAttachment->m_dlgGEMSeControl, newAttachment->m_dlgSamplings, Qt::Vertical);
	return newAttachment;
}

bool iAGEMSeAttachment::LoadSampling(QString const & smpFileName, int labelCount, int datasetID)
{
	return m_dlgGEMSeControl->LoadSampling(smpFileName, labelCount, datasetID);
}

bool iAGEMSeAttachment::LoadClustering(QString const & fileName)
{
	return m_dlgGEMSeControl->LoadClustering(fileName);
}

bool iAGEMSeAttachment::LoadRefImg(QString const & refImgName)
{
	return m_dlgGEMSeControl->LoadRefImg(refImgName);
}

void iAGEMSeAttachment::SetSerializedHiddenCharts(QString const & hiddenCharts)
{
	return m_dlgGEMSeControl->SetSerializedHiddenCharts(hiddenCharts);
}

void iAGEMSeAttachment::ResetFilter()
{
	m_dlgGEMSe->ResetFilters();
}

void iAGEMSeAttachment::ToggleAutoShrink()
{
	m_dlgGEMSe->ToggleAutoShrink();
}

void iAGEMSeAttachment::ToggleDockWidgetTitleBar()
{
	QWidget* titleBar = m_dlgGEMSe->titleBarWidget();
	if (titleBar == m_dummyTitleWidget)
	{
		m_dlgGEMSe->setTitleBarWidget(nullptr);
	}
	else
	{
		m_dlgGEMSe->setTitleBarWidget(m_dummyTitleWidget);
	}
}


void iAGEMSeAttachment::ExportClusterIDs()
{
	m_dlgGEMSeControl->ExportIDs();
}


void iAGEMSeAttachment::ExportAttributeRangeRanking()
{
	m_dlgGEMSeControl->ExportAttributeRangeRanking();
}

void iAGEMSeAttachment::ExportRankings()
{
	m_dlgGEMSeControl->ExportRankings();
}

void iAGEMSeAttachment::ImportRankings()
{
	m_dlgGEMSeControl->ImportRankings();
}

void iAGEMSeAttachment::SetLabelInfo(QString const & colorTheme, QString const & labelNames)
{
	m_dlgGEMSeControl->SetLabelInfo(colorTheme, labelNames);
}
